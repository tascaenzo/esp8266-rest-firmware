#!/usr/bin/env python3
import http.server
import socketserver
import os
import time
import threading
import base64
import hashlib
import struct

PORT = 8080
WEB_ROOT = os.path.abspath(".")
clients = set()
lock = threading.Lock()

# ---------- FILE WATCH ----------

WATCH_EXT = (".html", ".css", ".js")

def compute_stamp():
    latest = 0
    for root, _, files in os.walk(WEB_ROOT):
        for f in files:
            if not f.endswith(WATCH_EXT):
                continue
            try:
                latest = max(latest, os.path.getmtime(os.path.join(root, f)))
            except:
                pass
    return latest

def watch_files():
    last = compute_stamp()
    while True:
        now = compute_stamp()
        if now > last:
            last = now
            broadcast_reload()
        time.sleep(0.5)

# ---------- WEBSOCKET ----------

socketserver.ThreadingTCPServer.allow_reuse_address = True

def ws_handshake(key):
    GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    return base64.b64encode(
        hashlib.sha1((key + GUID).encode()).digest()
    ).decode()

def ws_frame(msg):
    payload = msg.encode()
    return b"\x81" + bytes([len(payload)]) + payload

def broadcast_reload():
    print("reload")
    frame = ws_frame("reload")

    with lock:
        for c in list(clients):
            try:
                c.sendall(frame)
            except:
                clients.remove(c)

def ws_keepalive(sock):
    try:
        while True:
            hdr = sock.recv(2)
            if not hdr:
                break

            length = hdr[1] & 0x7F
            if length == 126:
                sock.recv(2)
            elif length == 127:
                sock.recv(8)

            mask = sock.recv(4)
            sock.recv(length)
    except:
        pass

# ---------- HTTP HANDLER ----------

class Handler(http.server.SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def __init__(self, *a, **kw):
        super().__init__(*a, directory=WEB_ROOT, **kw)

    def do_GET(self):
        if self.headers.get("Upgrade", "").lower() == "websocket":
            self.handle_ws()
        else:
            super().do_GET()

    def handle_ws(self):
        key = self.headers.get("Sec-WebSocket-Key")
        accept = ws_handshake(key)

        self.request.sendall(
            b"HTTP/1.1 101 Switching Protocols\r\n"
            b"Upgrade: websocket\r\n"
            b"Connection: Upgrade\r\n"
            b"Sec-WebSocket-Accept: " + accept.encode() + b"\r\n\r\n"
        )

        with lock:
            clients.add(self.request)

        ws_keepalive(self.request)

        with lock:
            clients.discard(self.request)

# ---------- MAIN ----------

def main():
    threading.Thread(target=watch_files, daemon=True).start()

    with socketserver.ThreadingTCPServer(("", PORT), Handler) as httpd:
        print(f"HTTP + WS → http://localhost:{PORT}")
        httpd.serve_forever()

if __name__ == "__main__":
    main()
