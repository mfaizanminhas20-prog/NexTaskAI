import speech_recognition as sr
import datetime
import os

def record_and_parse():
    r = sr.Recognizer()
    with sr.Microphone() as source:
        print("NexTask AI is Listening...")
        r.adjust_for_ambient_noise(source)
        try:
            audio = r.listen(source, timeout=5)
            text = r.recognize_google(audio).lower()
            
            today = datetime.date.today()
            day, month, year = today.day, today.month, today.year
            
            if "tomorrow" in text:
                tomorrow = today + datetime.timedelta(days=1)
                day, month, year = tomorrow.day, tomorrow.month, tomorrow.year
                title = text.replace("tomorrow", "").strip()
            else:
                title = text

            # Append to file in exact format
            with open("tasks.txt", "a") as f:
                f.write(f"{title.capitalize()}|VoiceAI|1|0|0|{day}|{month}|{year}\n")
            print("Voice Task Added Successfully!")
        except:
            print("Could not hear anything.")

if __name__ == "__main__":
    record_and_parse()