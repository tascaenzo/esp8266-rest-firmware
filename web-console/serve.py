#!/usr/bin/env python3
import http.server
import socketserver
import os
import sys

PORT = 8080
WEB_ROOT = os.path.join(os.path.dirname(__file__), ".")

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_ROOT, **kwargs)

def main():
    if not os.path.isdir(WEB_ROOT):
        print("ERROR: ./ directory not found")
        sys.exit(1)

    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"ESP8266 Firmware Console")
        print(f"Serving {WEB_ROOT}")
        print(f"Open http://localhost:{PORT}")
        print("Press CTRL+C to stop")
        httpd.serve_forever()

if __name__ == "__main__":
    main()
