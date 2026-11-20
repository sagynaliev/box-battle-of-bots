#!/usr/bin/env python3
"""Serve the ESP32 controls over locally trusted HTTPS on the arena Wi-Fi."""

import http.client
import ipaddress
import os
from pathlib import Path
import socket
import ssl
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit

ESP = "192.168.4.1"
PORT = 8443
MAX_BODY = 6000


def arena_address():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.connect((ESP, 80))
        address = sock.getsockname()[0]
    if ipaddress.ip_address(address) not in ipaddress.ip_network("192.168.4.0/24"):
        raise SystemExit("Connect this computer to BoxBot-Arena Wi-Fi first.")
    return address


def check_controller():
    try:
        connection = http.client.HTTPConnection(ESP, timeout=3)
        try:
            connection.request("GET", "/api/status")
            response = connection.getresponse()
            if response.status != 200 or b"winnerReady" not in response.read(512):
                raise OSError("unexpected controller response")
        finally:
            connection.close()
    except (OSError, http.client.HTTPException) as error:
        raise SystemExit(f"Could not reach the ArenaController: {error}") from error


def certificates(address):
    folder = Path(__file__).resolve().parent / ".arena-https"
    folder.mkdir(mode=0o700, exist_ok=True)
    os.umask(0o077)
    ca, ca_key = folder / "arena-ca.cer", folder / "arena-ca.key"
    certificate, key = folder / f"{address}.crt", folder / f"{address}.key"
    extensions = folder / "openssl.cnf"
    extensions.write_text(
        "[req]\nprompt=no\ndistinguished_name=dn\n[dn]\nCN=BoxBot Arena Local\n"
        "[root]\nbasicConstraints=critical,CA:TRUE,pathlen:0\n"
        "keyUsage=critical,keyCertSign,cRLSign\nsubjectKeyIdentifier=hash\n"
        "[server]\nbasicConstraints=critical,CA:FALSE\n"
        "keyUsage=critical,digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n"
        f"subjectAltName=IP:{address}\n"
    )

    def openssl(*args):
        subprocess.run(["openssl", *map(str, args)], check=True, capture_output=True)

    if not ca.exists() or not ca_key.exists():
        for path in folder.glob("192.168.4.*.crt"):
            path.unlink()
        pem = folder / "arena-ca.pem"
        openssl("req", "-new", "-x509", "-newkey", "rsa:2048", "-nodes", "-sha256",
                "-days", "365", "-config", extensions, "-extensions", "root",
                "-keyout", ca_key, "-out", pem)
        openssl("x509", "-in", pem, "-outform", "der", "-out", ca)
    pem = folder / "arena-ca.pem"
    if not pem.exists():
        openssl("x509", "-inform", "der", "-in", ca, "-out", pem)

    valid = certificate.exists() and key.exists() and subprocess.run(
        ["openssl", "x509", "-in", str(certificate), "-noout", "-checkend", "86400"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    ).returncode == 0
    if not valid:
        request = folder / "arena.csr"
        openssl("req", "-new", "-newkey", "rsa:2048", "-nodes", "-config", extensions,
                "-keyout", key, "-out", request)
        openssl("x509", "-req", "-in", request, "-CA", pem, "-CAkey", ca_key,
                "-CAcreateserial", "-days", "30", "-sha256", "-extfile", extensions,
                "-extensions", "server", "-out", certificate)
    return ca, certificate, key


class ArenaProxy(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def proxy(self):
        path = urlsplit(self.path)
        allowed = {"/", "/api/status"} if self.command == "GET" else {"/api/command", "/api/audio"}
        if path.path not in allowed or path.netloc:
            self.send_error(404)
            return
        if self.command == "POST" and not self.headers.get("X-Arena-Command"):
            self.send_error(403)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.send_error(400)
            return
        if not 0 <= length <= MAX_BODY:
            self.send_error(413)
            return
        body = self.rfile.read(length) if length else None
        headers = {
            name: self.headers[name] for name in ("Content-Type", "X-Arena-Command")
            if name in self.headers
        }
        # Forward only fixed arena routes to the fixed ESP32 address.
        connection = http.client.HTTPConnection(ESP, timeout=3)
        try:
            connection.request(self.command, self.path, body=body, headers=headers)
            response = connection.getresponse()
            status = response.status
            content_type = response.getheader("Content-Type")
            payload = response.read()
        except (OSError, http.client.HTTPException):
            self.send_error(502, "ESP32 is unavailable")
            return
        finally:
            connection.close()
        try:
            self.send_response(status)
            if content_type:
                self.send_header("Content-Type", content_type)
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
        except (BrokenPipeError, ConnectionResetError):
            pass

    do_GET = proxy
    do_POST = proxy

    def log_message(self, *_):
        pass


def main():
    address = arena_address()
    check_controller()
    try:
        ca, certificate, key = certificates(address)
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.stderr.decode(errors="replace")) from error
    with ThreadingHTTPServer((address, PORT), ArenaProxy) as server:
        server.socket.settimeout(10)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.minimum_version = ssl.TLSVersion.TLSv1_2
        context.load_cert_chain(certificate, key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        print(f"Open on your phone: https://{address}:{PORT}/", flush=True)
        print(f"Install and trust this certificate on your own phone: {ca}", flush=True)
        print("Keep this terminal open while using the microphone.", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
