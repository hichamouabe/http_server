import os

# Grab the cookie from the dynamically generated HTTP_COOKIE environment variable
cookie = os.environ.get("HTTP_COOKIE", "")

# 1. If there is NO cookie, we tell the client (browser) to save one.
if "session_id=" not in cookie:
    print("Set-Cookie: session_id=super_secret_12345; HttpOnly; Path=/")

# 2. Print required HTTP headers gap
print("Content-Type: text/html\r\n\r\n")

# 3. HTML Body
print("<html><head><title>CGI Cookie Test</title></head>")
print("<body style='font-family: sans-serif; text-align: center; margin-top: 50px;'>")
print("<h2>CGI Cookie Implementation Test</h2>")

if cookie:
    print(f"<div style='background: #d4edda; padding: 20px; border-radius: 10px; display: inline-block;'>")
    print(f"<h3 style='color: green;'>Success! I received your cookie:</h3>")
    print(f"<code>{cookie}</code>")
    print("</div>")
else:
    print(f"<div style='background: #fff3cd; padding: 20px; border-radius: 10px; display: inline-block;'>")
    print(f"<h3 style='color: #856404;'>No Cookie Found!</h3>")
    print(f"<p>I just sent a <strong>Set-Cookie</strong> header back to your browser.</p>")
    print(f"<p>Refresh this page, and the browser should send it back to me!</p>")
    print("</div>")

print("</body></html>")
