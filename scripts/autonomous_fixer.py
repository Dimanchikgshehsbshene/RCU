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
MODEL = "openrouter/free"

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

def ask_ai(prompt, system_prompt):
    url = "https://openrouter.ai/api/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {OPENROUTER_API_KEY}",
        "Content-Type": "application/json",
        "HTTP-Referer": f"https://github.com/{REPO}",
        "X-Title": "Autonomous AI Fixer"
    }
    payload = {
        "model": MODEL,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": prompt}
        ],
        "response_format": { "type": "json_object" }
    }
    
    try:
        response = requests.post(url, headers=headers, data=json.dumps(payload))
        data = response.json()
        
        if "choices" not in data:
            print(f"API Error Response: {json.dumps(data, indent=2)}")
            if "error" in data:
                print(f"Error Message: {data['error'].get('message')}")
            return None
            
        return data["choices"][0]["message"]["content"]
    except Exception as e:
        print(f"Request failed: {str(e)}")
        return None

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
    """
    
    user_prompt = f"Analyze these failed logs and provide a fix by returning the FULL corrected file content:\n\n{logs}"
    
    print("🧠 Asking AI for a solution...")
    ai_content = ask_ai(user_prompt, system_prompt)
    
    if not ai_content:
        print("❌ AI failed to provide a response.")
        return

    try:
        ai_response = json.loads(ai_content)
    except json.JSONDecodeError:
        print(f"❌ AI returned invalid JSON: {ai_content}")
        return
    
    explanation = ai_response.get("explanation")
    content = ai_response.get("patch")
    target_file = ai_response.get("target_file")
    
    if content and target_file:
        print(f"🛠 Applying fix to {target_file}: {explanation}")
        
        os.makedirs(os.path.dirname(target_file), exist_ok=True)
        with open(target_file, "w") as f:
            f.write(content)
            
        run_command(f'git config --global user.name "AI Autonomous Fixer"')
        run_command(f'git config --global user.email "ai-fixer@manus.im"')
        run_command(f"git add {target_file}")
        run_command(f'git commit -m "AI Autonomous Fix: {explanation}"')
        
        remote_url = f"https://x-access-token:{GITHUB_TOKEN}@github.com/{REPO}.git"
        run_command(f"git remote set-url origin {remote_url}")
        stdout, stderr, code = run_command(f"git push origin {REF}")
        
        if code == 0:
            print("✅ Successfully pushed fix to branch!")
        else:
            print(f"❌ Failed to push: {stderr}")
    else:
        print("⚠️ AI provided an explanation but no file changes.")
        print(f"Explanation: {explanation}")

if __name__ == "__main__":
    main()
