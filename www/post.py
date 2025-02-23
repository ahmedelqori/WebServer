#!/usr/bin/env python3

import os
import sys

def main():
    # Print headers first
    print("Content-Type: text/plain")
    print()  # Empty line to separate headers from body
    
    if os.environ.get("REQUEST_METHOD") == "POST":
        # Read content length from environment
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        
        # Read raw POST data from stdin
        post_data = sys.stdin.buffer.read(content_length)
        
        # Print environment variables for debugging
        print("Environment variables:")
        print(f"CONTENT_LENGTH: {content_length}")
        print(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'Not set')}")
        print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'Not set')}")
        print("\nReceived POST data:")
        print(post_data)
    else:
        print("This script only handles POST requests.")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        # Error handling
        print("CGI Error occurred:")
        print(str(e))
        sys.exit(1)