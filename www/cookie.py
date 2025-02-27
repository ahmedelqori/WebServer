#!/usr/bin/env python3

import os

cookie_header = os.getenv("HTTP_COOKIE", "")
cookies = {}
if cookie_header:
    for cookie in cookie_header.split(";"):
        if "=" in cookie:
            name, value = cookie.strip().split("=", 1)
            cookies[name] = value
# Print the CGI headers
# Set outgoing cookies only if they don't already exist
if "username" not in cookies:
    print("Set-Cookie: username=JohnDoe; Path=/; HttpOnly")
if "theme" not in cookies:
    print("Set-Cookie: theme=dark; Path=/; Max-Age=3600")
print("Content-Type: text/html\n")

# Read incoming cookies


# Generate the response
print("<html>")
print("<head><title>Cookie Test</title></head>")
print("<body>")
print("<h1>Cookie Test</h1>")

# Display incoming cookies
if cookies:
    print("<h2>Incoming Cookies:</h2>")
    print("<ul>")
    for name, value in cookies.items():
        print(f"<li><strong>{name}:</strong> {value}</li>")
    print("</ul>")
else:
    print("<p>No incoming cookies.</p>")

# Confirm outgoing cookies
print("<h2>Outgoing Cookies:</h2>")
print("<p>Cookies have been set:</p>")
print("<ul>")
if "username" not in cookies:
    print("<li><strong>username:</strong> JohnDoe</li>")
if "theme" not in cookies:
    print("<li><strong>theme:</strong> dark</li>")
print("</ul>")

print("</body>")
print("</html>")