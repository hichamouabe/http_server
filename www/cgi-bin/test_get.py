import os

# 1. CGI Requires HTTP headers first
print("Content-Type: text/html\r\n\r\n")

print("<html><head><title>GET Test</title></head><body>")
print("<h1>GET Request Successful!</h1>")

# 2. Test Environment Variables
query = os.environ.get("QUERY_STRING", "None")
method = os.environ.get("REQUEST_METHOD", "Unknown")
print(f"<p><strong>Method:</strong> {method}</p>")
print(f"<p><strong>Query String:</strong> {query}</p>")

# 3. Test Relative Path Access (Evaluation Requirement!)
try:
    # Notice we just open "secret.txt", not the full absolute path!
    with open("secret.txt", "r") as f:
        data = f.read()
        print(f"<h3>Local File Read Test:</h3><p style='color:green;'>{data}</p>")
except Exception as e:
    print(f"<p style='color:red;'>Failed to read local file: {e}</p>")

print("</body></html>")
