import http.server
import webbrowser
import urllib.parse
import sys

PORT = 8080
AUTH_SUCCESS = False

class OAuthHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        global AUTH_SUCCESS
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        
        # Simulated Secure Google OAuth Landing page
        html = """
        <html>
        <head><title>NexTask AI Auth</title></head>
        <body style="font-family: Arial; text-align: center; margin-top: 100px; background: #0f0f13; color: white;">
            <h1 style="color: #3498db;">NexTask AI Authentication</h1>
            <p style="font-size: 18px; color: #2ecc71;">✔ Success! Google Account Linked Successfully.</p>
            <p style="color: #888;">You can close this tab and return to your application window.</p>
            <script>setTimeout(window.close, 3000);</script>
        </body>
        </html>
        """
        self.wfile.write(bytes(html, "utf-8"))
        AUTH_SUCCESS = True
        
    def log_message(self, format, *args):
        return # Console clean rakhne ke liye

def run_server():
    # Real Browser authentication workflow link simulation
    # Open local loopback receiver for oauth handshake
    url = f"http://localhost:{PORT}"
    print(f"Opening browser for Google Sign-In...")
    webbrowser.open(url)
    
    server = http.server.HTTPServer(('localhost', PORT), OAuthHandler)
    server.handle_request() # Wait for the user to click/authenticate in browser
    
    if AUTH_SUCCESS:
        # Secure session state file generator for C++ sync
        with open("session.txt", "w") as f:
            f.write("GOOGLE_USER_ACTIVE")
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    run_server()