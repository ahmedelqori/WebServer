#!/usr/bin/env python3

import cgi
import sys
import os

print("Content-Type: text/plain\n")

form = cgi.FieldStorage()

if os.environ['REQUEST_METHOD'] == 'POST':
	post_data = form.value
	print("Received POST data:\n")
	print(post_data)
else:
	print("This script only handles POST requests.")
