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
REF = os.getenv("GITHUB_REF_NAME")

# OPTIMIZED MODELS: Prioritizing reliable agentic models first
MODELS = [
    "openrouter/owl-alpha",                   # Top priority: Stable & Reliable
    "meta-llama/llama-3.3-70b-instruct:free", # High intelligence
    "qwen/qwen-2.5-72b-instruct:free",        # Strong coder
    "poolside/laguna-m.1:free",               # Specialized coding model
    "google/gemini-2.0-flash-exp:free",       # Fast and smart
    "deepseek/deepseek-v4-flash:free",        # Fast DeepSeek
    "openai/gpt-oss-120b:free",               # Large MoE
    "z-ai/glm-4.5-air:free",                  # GLM 4.5
    "google/gemma-2-27b-it:free",             # Efficient Google model
    "openrouter/free"                         # Final fallback
]

def run_command(command, cwd=None):
    result = subprocess.run(command, shell=True, capture_output=True, text=True, cwd=cwd)
    return result.stdout.strip(), result.stderr.strip(), result.returncode

def get_failed_logs():
    if not RUN_ID: return "No run ID."
    headers = {"Authorization": f"token {GITHUB_TOKEN}"}
    url = f"https://api.github.com/repos/{REPO}/actions/runs/{RUN_ID}/jobs"
    try:
        response = requests.get(url, headers=headers)
        response.raise_for_status()
        jobs = response.json().get("jobs", [])
        
        logs = ""
        for job in jobs:
            if job["conclusion"] == "failure":
                job_id = job["id"]
                log_url = f"https://api.github.com/repos/{REPO}/actions/jobs/{job_id}/logs"
                log_response = requests.get(log_url, headers=headers)
                logs += f"\nJob: {job['name']}\nLogs:\n{log_response.text[-10000:]}\n"
        return logs
    except Exception as e:
        return f"Error fetching logs: {str(e)}"

def ask_ai(model, prompt, system_prompt):
    url = "https://openrouter.ai/api/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {OPENROUTER_API_KEY}",
        "Content-Type": "application/json",
        "HTTP-Referer": f"https://github.com/{REPO}",
        "X-Title": "Autonomous AI Fixer"
    }
    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": prompt}
        ],
        "response_format": { "type": "json_object" }
    }
    
    try:
        print(f"🤖 Trying model: {model}...")
        response = requests.post(url, headers=headers, data=json.dumps(payload))
        data = response.json()
        
        if "choices" not in data:
            error_msg = data.get("error", {}).get("message", "Unknown error")
            print(f"⚠️ Model {model} failed. Error: {error_msg}")
            return None
            
        content = data["choices"][0]["message"]["content"]
        
        # Validation: Check if the AI actually provided a patch
        try:
            parsed = json.loads(content)
            if not parsed.get("patch") or len(parsed.get("patch").strip()) < 10:
                print(f"⚠️ Model {model} provided an explanation but NO valid patch. Skipping...")
                return None
        except:
            pass
            
        return content
    except Exception as e:
        print(f"❌ Request failed for {model}: {str(e)}")
        return None



def trigger_build():
    if not REPO or not REF or not GITHUB_TOKEN:
        print("⚠️ Skipping build trigger: missing REPO/REF/GITHUB_TOKEN.")
        return

    headers = {
        "Authorization": f"token {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json"
    }
    url = f"https://api.github.com/repos/{REPO}/actions/workflows/build.yml/dispatches"
    payload = {"ref": REF}

    try:
        response = requests.post(url, headers=headers, data=json.dumps(payload))
        if response.status_code == 204:
            print(f"✅ Build workflow started for branch: {REF}")
        else:
            print(f"⚠️ Failed to start build workflow ({response.status_code}): {response.text}")
    except Exception as e:
        print(f"⚠️ Error while starting build workflow: {str(e)}")

def sanitize_path(path):
    """
    Sanitizes the path provided by AI to prevent PermissionError and ensure it stays within the repo.
    """
    prefixes_to_remove = ["/__w/RCU/RCU/", "/home/runner/work/RCU/RCU/"]
    for prefix in prefixes_to_remove:
        if path.startswith(prefix):
            path = path.replace(prefix, "", 1)
    
    path = path.lstrip("/")
    path = os.path.normpath(path).replace("../", "")
    return path

def main():
    if not OPENROUTER_API_KEY:
        print("❌ OPENROUTER_API_KEY is not set in GitHub Secrets!")
        return
    if not GITHUB_TOKEN:
        print("❌ GITHUB_TOKEN (or GH_PAT) is not set!")
        return

    print(f"🚀 Starting autonomous fix for branch: {REF}")
    logs = get_failed_logs()
    
    system_prompt = """
    You are an autonomous AI engineer. Your task is to fix errors in this repository.
    CRITICAL RULES:
    1. DO NOT break "Ryazha-Авто" or "VRR" components.
    2. Provide a clear fix.
    3. Return ONLY a JSON object: {"explanation": "why it failed", "patch": "FULL content of the file", "target_file": "path/to/file"}
    4. MANDATORY: The "patch" field MUST contain the FULL corrected content of the file. DO NOT leave it empty.
    5. IMPORTANT: target_file MUST be a relative path from the repository root (e.g., "Source/main.cpp").
    """
    
    user_prompt = f"Analyze these failed logs and provide a fix by returning the FULL corrected file content in the 'patch' field:\n\n{logs}"
    
    ai_content = None
    for model in MODELS:
        ai_content = ask_ai(model, user_prompt, system_prompt)
        if ai_content:
            print(f"✅ Model {model} successfully provided a solution with a patch.")
            break
        else:
            print(f"🔄 Switching to next model...")
    
    if not ai_content:
        print("❌ All AI models failed to provide a valid response with a patch.")
        return

    try:
        ai_response = json.loads(ai_content)
    except json.JSONDecodeError:
        print(f"❌ AI returned invalid JSON: {ai_content}")
        return
    
    explanation = ai_response.get("explanation")
    content = ai_response.get("patch")
    raw_target_file = ai_response.get("target_file")
    
    if content and raw_target_file:
        target_file = sanitize_path(raw_target_file)
        print(f"🛠 Applying fix to {target_file}: {explanation}")
        
        try:
            dir_name = os.path.dirname(target_file)
            if dir_name:
                os.makedirs(dir_name, exist_ok=True)
            
            with open(target_file, "w") as f:
                f.write(content)
                
            run_command(f'git config --global user.name "AI Autonomous Fixer"')
            run_command(f'git config --global user.email "ai-fixer@manus.im"')
            run_command(f"git add \"{target_file}\"")
            run_command(f'git commit -m "AI Autonomous Fix: {explanation}"')
            
            remote_url = f"https://x-access-token:{GITHUB_TOKEN}@github.com/{REPO}.git"
            run_command(f"git remote set-url origin {remote_url}")
            stdout, stderr, code = run_command(f"git push origin {REF}")
            
            if code == 0:
                print("✅ Successfully pushed fix to branch!")
                trigger_build()
            else:
                print(f"❌ Failed to push: {stderr}")
        except Exception as e:
            print(f"❌ Error applying fix: {str(e)}")
    else:
        print("⚠️ AI provided an explanation but no file changes.")
        print(f"Explanation: {explanation}")

if __name__ == "__main__":
    main()
