#!/usr/bin/env python3

import cgi
import os
import cgitb
import logging
from datetime import datetime

# Enable CGI traceback for debugging
cgitb.enable()

# Set up logging
logging.basicConfig(filename='/home/mbentahi/Desktop/testserv/www/about', level=logging.INFO,
                    format='%(asctime)s - %(levelname)s - %(message)s')

# Directory to store uploaded files
UPLOAD_DIR = "/home/mbentahi/Desktop/testserv/www/test"

# Ensure the upload directory exists
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

# Print the content type header
print("Content-Type: text/html\n\n")

# Parse the form data
form = cgi.FieldStorage()

# Check if any files were uploaded
if "file" not in form:
    print("<h1>No file uploaded</h1>")
else:
    # Handle multiple file uploads
    file_items = form.getlist("file")
    for file_item in file_items:
        if file_item.filename:
            # Generate a unique filename to avoid overwriting
            filename = os.path.basename(file_item.filename)
            unique_filename = f"{datetime.now().strftime('%Y%m%d%H%M%S')}_{filename}"
            file_path = os.path.join(UPLOAD_DIR, unique_filename)

            try:
                # Save the uploaded file
                with open(file_path, "wb") as f:
                    f.write(file_item.file.read())
                print(f"<h1>File '{filename}' uploaded successfully as '{unique_filename}'!</h1>")
                logging.info(f"File '{filename}' uploaded successfully as '{unique_filename}'")
            except Exception as e:
                print(f"<h1>Error uploading file '{filename}': {e}</h1>")
                logging.error(f"Error uploading file '{filename}': {e}")
        else:
            print("<h1>No file selected</h1>")
            logging.warning("No file selected in one of the upload fields")

# Print form data (excluding file data)
print("<pre>")
print("Form data received (excluding file data):")
for key in form.keys():
    if key != "file":  # Skip printing the binary file data
        print(f"{key}: {form[key].value}")
print("</pre>")