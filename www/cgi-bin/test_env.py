# www/cgi-bin/test_env.py
import os

print("Content-Type: text/html\r\n\r\n")
print("<html><head><title>Environment Variables</title></head><body>")
print("<h2>CGI Environment Variables:</h2>")
print("<table border='1' style='border-collapse: collapse;'>")
print("<tr><th>Key</th><th>Value</th></tr>")

for key, value in sorted(os.environ.items()):
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")

print("</table></body></html>")
