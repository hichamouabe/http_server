# www/cgi-bin/test_large.py
import sys

print("Content-Type: text/html\r\n\r\n")
print("<html><body><h1>Large Output Test</h1>")

# Generate ~1MB of text
chunk = "This is a massive line of text designed to fill the pipe buffer. " * 20
print("<p>")
for i in range(1000):
    print(f"Line {i}: {chunk}<br>")
print("</p>")

print("</body></html>")
