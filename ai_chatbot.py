import sys
import os
import requests
import json

# Real-time Working Groq Key injected safely
API_KEY = "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"
URL = "https://api.groq.com/openai/v1/chat/completions"

def load_context_tasks():
    tasks_summary = []
    if os.path.exists("tasks_google_user.txt"):
        with open("tasks_google_user.txt", "r") as f:
            for line in f:
                parts = line.strip().split('|')
                if len(parts) >= 8:
                    status = "Completed" if parts[3] == "1" else "Pending"
                    tasks_summary.append(f"- Task: {parts[0]}, Category: {parts[1]}, Priority: {parts[2]} (0=Low,1=Med,2=High), Status: {status}, Progress: {parts[4]}%, Due: {parts[5]}/{parts[6]}/{parts[7]}")
    return "\n".join(tasks_summary) if tasks_summary else "No tasks currently in the manager."

def ask_groq(user_question):
    tasks_context = load_context_tasks()
    
    system_prompt = f"You are NexTask AI, an intelligent personal workspace assistant.\n" \
                    f"Here is the user's current live task database:\n{tasks_context}\n\n" \
                    f"Respond concisely, conversationally, and smartly in Roman Urdu/English blend. Use the context of their tasks to answer accurately. Keep the reply short (under 3 sentences) so it fits beautifully in the UI window."

    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }
    
    payload = {
        "model": "llama-3.1-8b-instant",  # Verified active free-tier model production ID
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_question}
        ],
        "max_tokens": 150
    }
    
    try:
        response = requests.post(URL, headers=headers, data=json.dumps(payload), timeout=15)
        if response.status_code == 200:
            result = response.json()
            reply = result['choices'][0]['message']['content']
            return reply.strip()
        else:
            return f"Groq Server Error Code: {response.status_code}. Data stream mismatch."
    except Exception as e:
        return f"Handshake Exception or Timeout: {str(e)[:35]}"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        query = sys.argv[1]
        ai_response = ask_groq(query)
        with open("chat_response.txt", "w", encoding="utf-8") as f:
            f.write(ai_response)