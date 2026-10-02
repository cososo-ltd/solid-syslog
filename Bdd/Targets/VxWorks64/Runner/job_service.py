#!/usr/bin/env python3
"""Job service for the VxWorks 6.4 runner.

Runs on the development machine. Jobs are queued here and the runner on the
build machine collects them, one at a time, connecting outwards only.
"""

import hmac
import http.server
import json


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

    def state(self, job_id):
        return self._states[job_id]

    def summary(self, job_id):
        return self._summaries[job_id]


def make_server(queue, token, host, port):
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            if not hmac.compare_digest(self.headers.get("X-Runner-Token", ""), token):
                self.send_response(401)
            else:
                self.send_response(204)
            self.end_headers()

        def do_POST(self):
            request = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
            job_id = queue.submit(request["type"], request["args"])
            body = json.dumps({"id": job_id}).encode()
            self.send_response(201)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

    return http.server.HTTPServer((host, port), Handler)
