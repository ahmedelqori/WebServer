#!/usr/bin/env python3

import os
import sys
import cgi

# Set the content-type header
print("Content-Type: text/plain\n")  # The blank line is important

# Get environment variables
request_method = os.environ.get("REQUEST_METHOD", "GET")
script_name = os.environ.get("SCRIPT_NAME", "")
query_string = os.environ.get("QUERY_STRING", "")

# Print some debug information (optional)
print(f"Request Method: {request_method}")
print(f"Script Name: {script_name}")
print(f"Query String: {query_string}")

# If there's any POST data, print it (for POST requests)
if request_method == "POST":
    form = cgi.FieldStorage()
    content_length = os.environ.get("CONTENT_LENGTH", "0")
    print(f"Content-Length: {content_length}")
    for key in form.keys():
        print(f"{key}: {form.getvalue(key)}")

# Output the main response content
print("\nHello, World! This is a CGI script response.")
