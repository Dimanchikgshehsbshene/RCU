#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
import textwrap
from pathlib import Path

import requests

API = "https://api.github.com"
OPENROUTER_URL = "https://openrouter.ai/api/v1/chat/completions"


def env(name: str, default: str = "") -> str:
    return os.environ.get(name, default).strip()


def gh_request(token: str, method: str, url: str, **kwargs):
    headers = kwargs.pop("headers", {})
    headers.update({
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    })
    r = requests.request(method, url, headers=headers, timeout=60, **kwargs)
    r.raise_for_status()
    return r


def collect_failed_logs(repo: str, run_id: str, token: str) -> str:
    jobs = gh_request(token, "GET", f"{API}/repos/{repo}/actions/runs/{run_id}/jobs").json().get("jobs", [])
    failed = [j for j in jobs if j.get("conclusion") == "failure"]
    if not failed:
        return "No failed jobs found."

    chunks = []
    for job in failed:
        name = job.get("name", "unknown")
        logs_url = job.get("logs_url")
        txt = f"\n=== Failed job: {name} ===\n"
        if logs_url:
            lr = gh_request(token, "GET", logs_url)
            # GitHub returns gzipped bytes for logs url; keep best-effort text.
            body = lr.content.decode("utf-8", errors="ignore")
            body = body[-12000:]
            txt += body
        else:
            txt += "No logs URL."
        chunks.append(txt)
    return "\n".join(chunks)


def ask_openrouter(key: str, prompt: str) -> str:
    payload = {
        "model": "openai/gpt-4o-mini",
        "messages": [
            {"role": "system", "content": "You are a CI auto-fix assistant. Return concise actionable steps and minimal patch suggestions."},
            {"role": "user", "content": prompt},
        ],
        "temperature": 0.2,
    }
    r = requests.post(
        OPENROUTER_URL,
        headers={
            "Authorization": f"Bearer {key}",
            "Content-Type": "application/json",
        },
        data=json.dumps(payload),
        timeout=120,
    )
    r.raise_for_status()
    data = r.json()
    return data["choices"][0]["message"]["content"]


def maybe_run_known_fix(logs: str) -> list[str]:
    ran = []
    if "ldr_process_creation" in logs or "include" in logs.lower():
        fixer = Path("scripts/fix_atmosphere_loader_includes.py")
        if fixer.exists():
            subprocess.run([sys.executable, str(fixer)], check=False)
            ran.append("scripts/fix_atmosphere_loader_includes.py")
    return ran


def main():
    token = env("GITHUB_TOKEN")
    openrouter = env("OPENROUTER_API_KEY")
    run_id = env("FAILED_RUN_ID")
    repo = env("GITHUB_REPOSITORY")

    if not token or not run_id or not repo:
        print("Missing required env vars: GITHUB_TOKEN, FAILED_RUN_ID, GITHUB_REPOSITORY")
        return 1

    logs = collect_failed_logs(repo, run_id, token)
    ran_fixers = maybe_run_known_fix(logs)

    analysis = "OPENROUTER_API_KEY missing; AI analysis skipped."
    if openrouter:
        prompt = textwrap.dedent(f"""
        Repository: {repo}
        Failed workflow run id: {run_id}

        Failed logs (tail):
        {logs}

        Provide:
        1) Root cause summary
        2) Exact files to change
        3) Minimal patch guidance
        """)
        try:
            analysis = ask_openrouter(openrouter, prompt)
        except Exception as e:
            analysis = f"AI request failed: {e}"

    report = Path("autofix_report.md")
    report.write_text(
        "# Autonomous Fix Report\n\n"
        f"- Repo: `{repo}`\n"
        f"- Failed run: `{run_id}`\n"
        f"- Applied local fixers: `{', '.join(ran_fixers) if ran_fixers else 'none'}`\n\n"
        "## AI Analysis\n\n"
        f"{analysis}\n",
        encoding="utf-8",
    )

    subprocess.run(["git", "add", "-A"], check=False)
    status = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True, check=False).stdout.strip()
    if not status:
        print("No changes produced by autonomous fixer.")
        return 0

    subprocess.run(["git", "commit", "-m", f"chore(autofix): attempt fix for failed run {run_id}"], check=False)
    subprocess.run(["git", "push", "origin", "HEAD"], check=False)
    print("Autonomous fix commit pushed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
