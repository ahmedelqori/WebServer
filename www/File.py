#!C:\Python311\python.exe

import cgi, cgitb

cgitb.enable()  # Enable error reporting

try:
    form = cgi.FieldStorage()

    username = form.getvalue("username", "")
    emailaddress = form.getvalue("emailaddress", "")

    print("Content-type:text/html\r\n\r\n")

    print("<html>")
    print("<head>")
    print("<title> MY FIRST CGI FILE </title>")
    print("</head>")
    print("<body>")
    print("<h3> This is HTML's Body Section </h3>")
    print(f"Username: {username}<br>")
    print(f"Email Address: {emailaddress}")
    print("</body>")
    print("</html>")
except Exception as e:
    print("Content-type:text/html\r\n\r\n")
    print("<html>")
    print("<head>")
    print("<title> CGI Error </title>")
    print("</head>")
    print("<body>")
    print("<h3> CGI Error </h3>")
    print(f"Error: {e}")
    print("</body>")
    print("</html>")