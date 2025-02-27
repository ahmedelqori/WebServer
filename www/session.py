#!/usr/bin/env python3

import os
import cgitb
import uuid
import cgi
import time
import sys

import urllib.parse

def generateId():
    return str(uuid.uuid4())

def generateExpirationDate():
    expiration_time = time.time() + 60 * 60 * 24 * 30
    formatted_time = time.strftime("%a, %d-%b-%Y %H:%M:%S GMT", time.gmtime(expiration_time))
    return formatted_time

def createNewCookie():
    user_id = generateId()
    expiration_date = generateExpirationDate()
    # Return the cookie header line rather than printing immediately
    cookieLine = "Set-Cookie: session_id=" + user_id + "; Expires=" + expiration_date + "; Path=/\r\n"
    return user_id, cookieLine

# Enable CGI traceback
cgitb.enable()

# Collect header lines in a list
headers = []

content_type = "Content-Type: text/html\r\n"
headers.append(content_type)

# Ensure session cookie exists
cookies = os.environ.get('HTTP_COOKIE', '')
user_id = None
session_cookie_header = None
if cookies == "":
    user_id, session_cookie_header = createNewCookie()
    headers.append(session_cookie_header)
else:
    for cookie in cookies.split(';'):
        try:
            name, value = cookie.strip().split('=', 1)
            if name == "session_id":
                user_id = value
                break
        except ValueError:
            continue
if user_id is None:
    user_id, session_cookie_header = createNewCookie()
    headers.append(session_cookie_header)

# Handle theme using cookies instead of file
raw_data = sys.stdin.read() if sys.stdin is not None else ""

parsed_data = urllib.parse.parse_qs(raw_data)
form_theme = parsed_data.get("theme", [None])[0]

existing_theme = None
if cookies:
    for cookie in cookies.split(';'):
        try:
            name, val = cookie.strip().split('=', 1)
            if name == "theme":
                existing_theme = val
                break
        except ValueError:
            continue

if form_theme:
    theme = form_theme
    expiration_date = generateExpirationDate()
    theme_cookie = "Set-Cookie: theme=" + theme + "; Expires=" + expiration_date + "; Path=/\r\n\r\n"
    headers.append(theme_cookie)
else:
    theme = existing_theme if existing_theme in ["light", "dark"] else "light"
    if not existing_theme:
        expiration_date = generateExpirationDate()
        theme_cookie = "Set-Cookie: theme=" + theme + "; Expires=" + expiration_date + "; Path=/\r\n\r\n"
        headers.append(theme_cookie)

if theme == "dark":
    background_color = "#333"
    text_color = "#f0f0f0"
    buttonColor = "white"
else:
    background_color = "#f0f0f0"
    text_color = "#333"
    buttonColor = "black"

light_selected = "selected" if theme == "light" else ""
dark_selected = "selected" if theme == "dark" else ""

# Print all headers with proper delimiter and then the HTML content.
print("".join(headers))
print("\r\n")


html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Informations de l'Utilisateur</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap');
        body {{
            font-family: 'Inter', sans-serif;
            background-color: {background_color};
            color: {text_color};
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }}
        .container {{
            text-align: center;
        }}
        .button {{
            margin-top: 2rem;
            border: none;
            padding: 1rem 3rem;
            background-color: {buttonColor};
            border-radius: 15px;
            font-weight: semibold;
            color: {background_color};
        }}
        form {{
            display: flex;
            flex-direction: column;
            align-items: center;
        }}
        #theme {{
            margin-top: 1rem;
            padding: 5px 20px;
            background-color: {text_color};
            border-radius: 3px;
            font-weight: semibold;
            color: {background_color};
        }}
        #theme option {{
            display: flex;
        }}    
    </style>
</head>
<body>
    <div class="container">
        <h2>Cookie Page</h2>
        <form action="/session.py" method="post">
            <label for="theme">Choose a theme :</label>
            <select name="theme" id="theme">
                <option {light_selected} value="light">Light</option>
                <option {dark_selected} value="dark">Dark</option>
            </select>
            <button class="button" type="submit">Save</button>
        </form>
    </div>
</body>
<script src="https://cdn.jsdelivr.net/npm/js-confetti@latest/dist/js-confetti.browser.js"></script>
<script>
    const jsConfetti = new JSConfetti()
    jsConfetti.addConfetti({{ confettiNumber: 1 }})
</script>
</html>
"""

print(html_content)