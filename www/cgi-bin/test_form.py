# www/cgi-bin/test_form.py
import os
import sys
import urllib.parse

print("Content-Type: text/html\r\n\r\n")

method = os.environ.get("REQUEST_METHOD", "GET")

print("<html><head><title>CGI Form</title></head><body>")
print("<h2>Simple CGI Form</h2>")

if method == "POST":
    # Read the URL-encoded data from STDIN
    body = sys.stdin.read()
    parsed = urllib.parse.parse_qs(body)
    
    name = parsed.get("name", ["Unknown"])[0]
    message = parsed.get("message", ["No message"])[0]
    
    print("<div style='background-color:#d4edda; padding:10px; border-radius:5px;'>")
    print(f"<strong>Success!</strong> Received data:<br>")
    print(f"Name: {name}<br>")
    print(f"Message: {message}")
    print("</div><br>")

print("""
    <form method="POST" action="/cgi-bin/test_form.py">
        <label>Name:</label><br>
        <input type="text" name="name" required><br><br>
        
        <label>Message:</label><br>
        <textarea name="message" required></textarea><br><br>
        
        <input type="submit" value="Submit via POST">
    </form>
""")
print("</body></html>")
