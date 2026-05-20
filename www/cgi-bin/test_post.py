import sys
import os

print("Content-Type: text/html\r\n\r\n")

print("<html><body>")
print("<h1>POST Request Successful!</h1>")

method = os.environ.get("REQUEST_METHOD", "Unknown")
content_len = os.environ.get("CONTENT_LENGTH", "0")

print(f"<p><strong>Method:</strong> {method}</p>")
print(f"<p><strong>Reported Content Length:</strong> {content_len}</p>")

# Test STDIN reading (Webserv attached the tmpfile here)
body = sys.stdin.read()

print("<h3>Body received by CGI:</h3>")
print(f"<pre style='background:#eee; padding:10px;'>{body}</pre>")
print("</body></html>")
