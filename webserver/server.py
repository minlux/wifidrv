#!/usr/bin/env python3
"""
Image webserver for the wifidrv picture frame.

Serves images from the assets/ subfolder in round-robin order.
Each GET request — regardless of path — returns the next image.
"""

import argparse
import os
import random
from http.server import BaseHTTPRequestHandler, HTTPServer

ASSETS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets")

CONTENT_TYPES = {
    ".jpg":  "image/jpeg",
    ".jpeg": "image/jpeg",
    ".png":  "image/png",
    ".gif":  "image/gif",
    ".bmp":  "image/bmp",
}

_images = []
_index  = 0


def load_images():
    global _images
    _images = sorted(
        f for f in os.listdir(ASSETS_DIR)
        if os.path.splitext(f)[1].lower() in CONTENT_TYPES
    )


def next_image():
    global _index
    if not _images:
        return None, None, None
    name  = _images[_index % len(_images)]
    _index += 1
    path  = os.path.join(ASSETS_DIR, name)
    ctype = CONTENT_TYPES[os.path.splitext(name)[1].lower()]
    return name, path, ctype


def random_image():
    if not _images:
        return None, None, None
    name  = random.choice(_images)
    path  = os.path.join(ASSETS_DIR, name)
    ctype = CONTENT_TYPES[os.path.splitext(name)[1].lower()]
    return name, path, ctype


def serve_image(handler, name, path, ctype):
    with open(path, "rb") as f:
        data = f.read()

    handler.send_response(200)
    handler.send_header("Content-Type",   ctype)
    handler.send_header("Content-Length", str(len(data)))
    handler.end_headers()
    handler.wfile.write(data)

    print(f"  {handler.client_address[0]}  {handler.path}  ->  {name}  ({len(data):,} bytes)")


class ImageHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        path = self.path.rstrip("/")

        if path == "/picture-frame":
            name, fpath, ctype = random_image()
            if name is None:
                self.send_error(404, "No images in assets/")
                return
            serve_image(self, name, fpath, ctype)

        else:
            self.send_error(404, "Not found")

    def log_message(self, fmt, *args):
        pass  # suppress default apache-style access log


def main():
    parser = argparse.ArgumentParser(
        description="Image webserver for wifidrv picture frame"
    )
    parser.add_argument("--host", default="0.0.0.0",
                        help="Bind address (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=8080,
                        help="Port to listen on (default: 8080)")
    args = parser.parse_args()

    load_images()

    if not _images:
        print(f"WARNING: no images found in {ASSETS_DIR}")
    else:
        print(f"Images ({len(_images)}):")
        for name in _images:
            size = os.path.getsize(os.path.join(ASSETS_DIR, name))
            print(f"  {name}  ({size:,} bytes)")

    print(f"\nListening on {args.host}:{args.port}  (Ctrl+C to stop)\n")

    server = HTTPServer((args.host, args.port), ImageHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
