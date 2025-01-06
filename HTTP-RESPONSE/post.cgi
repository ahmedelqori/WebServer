#!/usr/bin/env python3

import cgi
import sys
import os

print("Content-Type: text/plain")
print()  # Empty line is required

try:
    form = cgi.FieldStorage()
    if os.environ['REQUEST_METHOD'] == 'POST':
        # Print all received form data
        for key in form.keys():
            print(f"{key}: {form.getvalue(key)}")
    else:
        print("This script only handles POST requests.")
except Exception as e:
    print("Error processing request:")
    print(str(e))