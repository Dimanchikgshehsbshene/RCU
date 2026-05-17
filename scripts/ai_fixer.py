import os
import requests
import json
import subprocess
import sys

# Configuration
OPENROUTER_API_KEY = os.getenv("OPENROUTER_API_KEY")
GITHUB_TOKEN = os.getenv("GITHUB_TOKEN")
REPO = os.getenv("GITHUB_REPOSITORY")
RUN_ID = os.getenv("FAILED_RUN_ID")
MODEL = "anthropic/claude-3.5-sonnet" # One of the smartest models available on OpenRouter

def run_command(command, cwd=None):
    result = subprocess.run(command, shell=True, capture_output=True, text=True, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed: {command}\nError: {result.stderr}")
    return result.stdout.strip()

def get_failed_logs():
    headers = {"Authorization": f"token {GITHUB_TOKEN}"}
    url = f"https://api.github.com/repos/{REPO}/actions/runs/{RUN_ID}/jobs"
    response = requests.get(url, headers=headers)
    jobs = response.json().get("jobs", [])
    
    logs = ""
    for job in jobs:
        if job["conclusion"] == "failure":
            job_id = job["id"]
            log_url = f"https://api.github.com/repos/{REPO}/actions/jobs/{job_id}/logs"
            log_response = requests.get(log_url, headers=headers)
            logs += f"\nJob: {job['name']}\nLogs:\n{log_response.text[-8000:]}\n"
    return logs

def get_repo_structure():
    return run_command("find . -maxdepth 2 -not -path '*/.*'")

def ask_ai(error_logs, structure):
    url = "https://openrouter.ai/api/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {OPENROUTER_API_KEY}",
        "Content-Type": "application/json",
        "HTTP-Referer": "https://github.com/Dimasick-git/RCU",
        "X-Title": "GitHub Actions AI Fixer"
    }
    
    prompt = f"""
    You are an expert software engineer. A GitHub Action failed in the repository {REPO}.
    Your task is to analyze the logs and provide a fix.
    
    Repository Structure:
    {structure}
    
    Failed Logs:
    {error_logs}
    
    Constraints:
    1. DO NOT break "Ryazha-Авто" or "VRR" components.
    2. Be extremely careful with Atmosphere and Libnx integrations.
    
    Instructions:
    - If the error is in a workflow file, provide the corrected YAML.
    - If the error is in the source code, provide the fix as a standard unified diff (patch).
    - Provide your response in JSON format:
    {{
        "explanation": "Brief explanation of the error and fix",
        "files_to_read": ["list", "of", "files", "you", "need", "to", "see", "to", "confirm", "the", "fix"],
        "patch": "The diff content or the full content of the file if it's a small file"
    }}
    
    If you need to see file contents first, just list them in "files_to_read" and leave "patch" empty.
    """
    
    payload = {
        "model": MODEL,
        "messages": [{"role": "user", "content": prompt}],
        "response_format": { "type": "json_object" }
    }
    
    response = requests.post(url, headers=headers, data=json.dumps(payload))
    return response.json()["choices"][0]["message"]["content"]

def main():
    if not OPENROUTER_API_KEY or not GITHUB_TOKEN:
        print("Missing API keys")
        return

    logs = get_failed_logs()
    structure = get_repo_structure()
    
    print("Initial AI analysis...")
    ai_response = json.loads(ask_ai(logs, structure))
    
    if ai_response.get("files_to_read"):
        print(f"AI requested files: {ai_response['files_to_read']}")
        context = ""
        for f in ai_response["files_to_read"]:
            if os.path.exists(f):
                with open(f, 'r') as file:
                    context += f"\nFile: {f}\nContent:\n{file.read()}\n"
        
        # Second pass with file content
        prompt_with_context = f"Here is the content of the files you requested:\n{context}\n\nNow provide the fix in the same JSON format."
        url = "https://openrouter.ai/api/v1/chat/completions"
        headers = {"Authorization": f"Bearer {OPENROUTER_API_KEY}", "Content-Type": "application/json"}
        payload = {
            "model": MODEL,
            "messages": [
                {"role": "user", "content": f"Logs: {logs}\nStructure: {structure}"},
                {"role": "assistant", "content": json.dumps(ai_response)},
                {"role": "user", "content": prompt_with_context}
            ],
            "response_format": { "type": "json_object" }
        }
        response = requests.post(url, headers=headers, data=json.dumps(payload))
        ai_response = json.loads(response.json()["choices"][0]["message"]["content"])

    if ai_response.get("patch"):
        print(f"Applying fix: {ai_response['explanation']}")
        
        # Save patch to file
        with open("fix.patch", "w") as f:
            f.write(ai_response["patch"])
        
        # Try to apply patch
        patch_result = run_command("patch -p1 < fix.patch")
        print(patch_result)
        
        # Create PR
        branch_name = f"ai-fix-{RUN_ID}"
        run_command(f"git checkout -b {branch_name}")
        run_command("git add .")
        run_command(f'git commit -m "AI Fix: {ai_response["explanation"]}"')
        run_command(f"git push origin {branch_name}")
        
        pr_body = f"AI generated fix for failed run {RUN_ID}.\n\n**Explanation:**\n{ai_response['explanation']}"
        run_command(f'gh pr create --title "AI Fix for Run {RUN_ID}" --body "{pr_body}" --base main --head {branch_name}')
    else:
        print("AI could not determine a fix.")

if __name__ == "__main__":
    main()
