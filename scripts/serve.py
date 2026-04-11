#!/usr/bin/env python3
"""
serve.py — Serve the flashcard app from the scripts/ directory.

Usage (run from repo root OR from scripts/):
    python scripts/serve.py          # port 8000
    python scripts/serve.py 8080     # custom port

Then open: http://localhost:8000/flashcards-ai.html
"""
import http.server
import socketserver
import webbrowser
import sys
import os
from pathlib import Path

# Always serve from the scripts/ directory, regardless of where this is run from
SCRIPTS_DIR = Path(__file__).parent.resolve()
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
URL = f"http://localhost:{PORT}/flashcards-ai.html"

os.chdir(SCRIPTS_DIR)

class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass  # suppress per-request logging

print(f"Serving from: {SCRIPTS_DIR}")
print(f"Opening:      {URL}")
print(f"Stop with:    Ctrl+C\n")

webbrowser.open(URL)

with socketserver.TCPServer(("", PORT), Handler) as httpd:
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
