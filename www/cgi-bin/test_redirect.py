# www/cgi-bin/test_redirect.py

# A CGI redirect is just a Location header (Status is technically optional, 
# but usually browsers like 302 Found).
print("Status: 302 Found\r\nLocation: https://profile.intra.42.fr/\r\n\r\n")
