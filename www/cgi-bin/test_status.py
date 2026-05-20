# www/cgi-bin/test_status.py
import sys

# Set a custom HTTP status code
print("Status: 418 I'm a teapot\r\nContent-Type: text/html\r\n\r\n")

print("<html><body>")
print("<h1>418 I'm a teapot</h1>")
print("<p>The CGI script requested a custom status code, and Webserv parsed it!</p>")
print("</body></html>")
