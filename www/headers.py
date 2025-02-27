def send_headers():
	# Status should always be the first header
	print("Status: 200 OK")
	
	# Content headers
	print("Content-Length: 222")
	print("Content-Type: text/html")
	# print("Content-Disposition: attachment; filename=pic.py")
	print("Status: 502 OK")
	print("Content-Length: 0")
	print("Content-Type: page/html")
	
	
	# Empty line to separate headers from body
	print("\n")

# Call the function
send_headers()
