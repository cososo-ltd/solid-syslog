#!/usr/bin/env python3
"""Job service for the VxWorks 6.4 runner.

Runs on the development machine. Jobs are queued here and the runner on the
build machine collects them, one at a time, connecting outwards only.

    python job_service.py init                  once: token, certificate and key
    python job_service.py serve                 serve on port 8765 until stopped
    python job_service.py submit <job> [NAME=VALUE...]
                                                queue a job, follow its log, and
                                                exit 0 only if it succeeded
    python job_service.py console               show the console of the QEMU a
                                                qemu-start job began, which
                                                connects out to port 8766

The token, certificate and key live in ~/.solidsyslog-runner, never in the
repository. serve prints the certificate's thumbprint, which the runner is
given when it is launched.
"""

import argparse
import hashlib
import hmac
import http.server
import json
import os
import re
import secrets
import shutil
import socket
import ssl
import subprocess
import sys
import time
import urllib.parse
import urllib.request

DEFAULT_HOME = os.path.join(os.path.expanduser("~"), ".solidsyslog-runner")
DEFAULT_PORT = 8765
DEFAULT_CONSOLE_PORT = 8766


class JobQueue:
    def __init__(self):
        self._pending = []
        self._states = {}
        self._summaries = {}
        self._logs = {}
        self._last_id = 0

    def submit(self, job_type, args):
        self._last_id += 1
        self._pending.append({"id": self._last_id, "type": job_type, "args": args})
        self._states[self._last_id] = "queued"
        return self._last_id

    def next(self):
        if self._pending:
            job = self._pending.pop(0)
            self._states[job["id"]] = "running"
            return job
        return None

    def append_log(self, job_id, text):
        self._logs[job_id] = self._logs.get(job_id, "") + text

    def log(self, job_id, offset):
        return self._logs.get(job_id, "")[offset:]

    def finish(self, job_id, outcome, summary):
        self._states[job_id] = outcome
        self._summaries[job_id] = summary

    def knows(self, job_id):
        return job_id in self._states

    def state(self, job_id):
        return self._states[job_id]

    def summary(self, job_id):
        return self._summaries.get(job_id)


# A log chunk is a few seconds of build output, far below the default limit.
def make_server(queue, token, host, port, certificate=None, max_body_bytes=4 * 1024 * 1024):
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            self._dispatch("GET")

        def do_POST(self):
            self._dispatch("POST")

        # The body is read before any reply: closing a connection with a body
        # still unread makes Windows abort it, and the client sees that instead
        # of the reply. One over the limit is refused unread, token or not.
        def _dispatch(self, method):
            length = int(self.headers.get("Content-Length", 0))
            if length > max_body_bytes:
                self.close_connection = True
                self._reply(413)
            else:
                self._request_body = self.rfile.read(length)
                self._route(method)

        # Every route but /jobs and /jobs/next names a job, which must exist.
        def _route(self, method):
            url = urllib.parse.urlsplit(self.path)
            action = None
            args = ()
            for route_method, pattern, route_action in ROUTES:
                match = re.fullmatch(pattern, url.path)
                if (route_method == method) and match:
                    args = tuple(int(group) for group in match.groups())
                    if all(queue.knows(job_id) for job_id in args):
                        action = route_action
                    break
            if not self._authorised():
                self._reply(401)
            elif action is None:
                self._reply(404)
            else:
                action(self, url, *args)

        def _post_job(self, url):
            request = json.loads(self._body())
            self._reply(201, {"id": queue.submit(request["type"], request["args"])})

        def _get_next(self, url):
            job = queue.next()
            if job is None:
                self._reply(204)
            else:
                self._reply(200, job)

        def _get_status(self, url, job_id):
            self._reply(200, {"state": queue.state(job_id), "summary": queue.summary(job_id)})

        def _get_log(self, url, job_id):
            offset = int(urllib.parse.parse_qs(url.query).get("from", ["0"])[0])
            self._reply_text(queue.log(job_id, offset))

        def _post_log(self, url, job_id):
            queue.append_log(job_id, self._body().decode())
            self._reply(204)

        def _post_result(self, url, job_id):
            result = json.loads(self._body())
            queue.finish(job_id, result["outcome"], result["summary"])
            self._reply(204)

        # The runner polls for work every few seconds; an empty poll is not news.
        def log_request(self, code="-", size="-"):
            if not ((code == 204) and (self.path == "/jobs/next")):
                super().log_request(code, size)

        def _authorised(self):
            return hmac.compare_digest(self.headers.get("X-Runner-Token", ""), token)

        def _body(self):
            return self._request_body

        def _reply(self, status, payload=None):
            self.send_response(status)
            if payload is None:
                self.end_headers()
            else:
                body = json.dumps(payload).encode()
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        def _reply_text(self, text):
            body = text.encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

    ROUTES = (
        ("POST", r"/jobs", Handler._post_job),
        ("GET", r"/jobs/next", Handler._get_next),
        ("GET", r"/jobs/(\d+)", Handler._get_status),
        ("GET", r"/jobs/(\d+)/log", Handler._get_log),
        ("POST", r"/jobs/(\d+)/log", Handler._post_log),
        ("POST", r"/jobs/(\d+)/result", Handler._post_result),
    )

    server = http.server.HTTPServer((host, port), Handler)
    if certificate is not None:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(*certificate)
        server.socket = context.wrap_socket(server.socket, server_side=True)
    return server


# The SHA-1 of the certificate in upper-case hex: what Windows calls its
# thumbprint, and what the runner pins.
def thumbprint(certificate_path):
    with open(certificate_path, encoding="ascii") as certificate:
        der = ssl.PEM_cert_to_DER_cert(certificate.read())
    return hashlib.sha1(der).hexdigest().upper()


# Submits a job, writes its log to out as it arrives, and returns its outcome
# and summary once the runner has finished it. The status is read before the
# log, so the last read of the log follows the runner's last write to it.
def run_job(base_url, token, job_type, args, out, poll_seconds=1.0, context=None):
    job_id = json.loads(_call(base_url, token, context, "POST", "/jobs", {"type": job_type, "args": args}))["id"]
    offset = 0
    finished = False
    while not finished:
        status = json.loads(_call(base_url, token, context, "GET", f"/jobs/{job_id}"))
        finished = status["state"] not in ("queued", "running")
        text = _call(base_url, token, context, "GET", f"/jobs/{job_id}/log?from={offset}").decode()
        out.write(text)
        offset += len(text)
        if not finished:
            time.sleep(poll_seconds)
    return status["state"], status["summary"]


def _call(base_url, token, context, method, path, payload=None):
    data = None if payload is None else json.dumps(payload).encode()
    request = urllib.request.Request(base_url + path, data=data, method=method)
    request.add_header("X-Runner-Token", token)
    with urllib.request.urlopen(request, context=context) as response:
        return response.read()


def job_arguments(words):
    return dict(word.split("=", 1) for word in words)


# Creates the service's token, and the self-signed certificate and key it
# serves, once. The runner pins the certificate by thumbprint, so the name in it
# matters to nobody and the address it is reached at may change.
def initialise(home):
    openssl = shutil.which("openssl")
    if openssl is None:
        raise RuntimeError("openssl was not found on the PATH - Git for Windows provides one")
    os.makedirs(home, exist_ok=True)
    with open(os.path.join(home, "token"), "x", encoding="ascii") as token:
        token.write(secrets.token_urlsafe(32))
    subprocess.run(
        [openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes",
         "-keyout", os.path.join(home, "key.pem"), "-out", os.path.join(home, "certificate.pem"),
         "-days", "3650", "-subj", "/CN=solidsyslog-runner"],
        check=True, capture_output=True)



# Accepts the target's console, which QEMU connects out to, and writes what it
# sends to out until it disconnects. Stopping QEMU kills it, so a reset is how
# the console usually ends.
def relay_console(listener, out):
    connection, _ = listener.accept()
    with connection:
        data = _receive(connection)
        while data:
            out.write(data)
            out.flush()
            data = _receive(connection)


def _receive(connection):
    try:
        return connection.recv(4096)
    except ConnectionResetError:
        return b""

def main(argv):
    parser = argparse.ArgumentParser(description="Job service for the VxWorks 6.4 runner.")
    parser.add_argument("--home", default=DEFAULT_HOME, help="where the token, certificate and key live")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("init", help="create the token, certificate and key")
    commands.add_parser("serve", help="serve until stopped")
    console = commands.add_parser("console", help="show the target's console when it connects")
    console.add_argument("--console-port", type=int, default=DEFAULT_CONSOLE_PORT)
    submit = commands.add_parser("submit", help="queue a job and follow it")
    submit.add_argument("job")
    submit.add_argument("arguments", nargs="*", metavar="NAME=VALUE")
    options = parser.parse_args(argv)

    certificate = os.path.join(options.home, "certificate.pem")
    key = os.path.join(options.home, "key.pem")
    result = 0
    if options.command == "console":
        with socket.create_server(("0.0.0.0", options.console_port)) as listener:
            print(f"Waiting for the target's console on port {options.console_port}", flush=True)
            relay_console(listener, sys.stdout.buffer)
    elif options.command == "init":
        initialise(options.home)
        print(f"Created the token, certificate and key in {options.home}")
        print(f"Thumbprint {thumbprint(certificate)}")
    else:
        with open(os.path.join(options.home, "token"), encoding="ascii") as token_file:
            token = token_file.read().strip()
        if options.command == "serve":
            server = make_server(JobQueue(), token, "0.0.0.0", options.port, (certificate, key))
            print(f"Serving on port {options.port}, thumbprint {thumbprint(certificate)}", flush=True)
            try:
                server.serve_forever()
            except KeyboardInterrupt:
                pass
        else:
            context = ssl.create_default_context(cafile=certificate)
            context.check_hostname = False
            outcome, summary = run_job(f"https://127.0.0.1:{options.port}", token, options.job,
                                       job_arguments(options.arguments), sys.stdout, context=context)
            print(f"{outcome}: {summary}")
            result = 0 if outcome == "succeeded" else 1
    return result


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
