#!/usr/bin/env python3

import cgi
import os
import cgitb
cgitb.enable()  # for debugging

# Directory where uploaded files will be stored
UPLOAD_DIR = "/home/mbentahi/Desktop/testserv/www/test"

# Ensure the upload directory exists
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

# Set the content type to HTML
print("Content-Type: text/html\n")

# Get the form data
form = cgi.FieldStorage()

# Check if a file was uploaded
if "file" not in form:
    print("<h1>No file uploaded</h1>")
else:
    file_item = form["file"]
    
    # Check if the file was uploaded successfully
    if file_item.filename:
        # Sanitize the filename
        filename = os.path.basename(file_item.filename)
        file_path = os.path.join(UPLOAD_DIR, filename)
        
        # Save the file to the upload directory
        with open(file_path, "wb") as f:
            f.write(file_item.file.read())
        
        print(f"<h1>File '{filename}' uploaded successfully!</h1>")
    else:
        print("<h1>No file selected</h1>")
        
print("<pre>")
print("Form data received:")
for key in form.keys():
    print(f"{key}: {form[key]}")
print("</pre>")