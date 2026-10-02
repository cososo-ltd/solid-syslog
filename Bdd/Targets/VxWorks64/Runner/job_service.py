#!/usr/bin/env python3
"""Job service for the VxWorks 6.4 runner.

Runs on the development machine. Jobs are queued here and the runner on the
build machine collects them, one at a time, connecting outwards only.
"""

import hashlib
import hmac
import http.server
import json
import os
import re
import secrets
import shutil
import ssl
import subprocess
import time
import urllib.parse
import urllib.request


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


def make_server(queue, token, host, port, certificate=None):
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            self._dispatch("GET")

        def do_POST(self):
            self._dispatch("POST")

        # Every route but /jobs and /jobs/next names a job, which must exist.
        def _dispatch(self, method):
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

        def _authorised(self):
            return hmac.compare_digest(self.headers.get("X-Runner-Token", ""), token)

        def _body(self):
            return self.rfile.read(int(self.headers["Content-Length"]))

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
