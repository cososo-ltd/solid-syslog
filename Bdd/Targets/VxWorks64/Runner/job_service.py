#!/usr/bin/env python3
"""Job service for the VxWorks 6.4 runner.

Runs on the development machine. Jobs are queued here and the runner on the
build machine collects them, one at a time, connecting outwards only.
"""

import hmac
import http.server
import json
import re
import urllib.parse


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
        return self._summaries.get(job_id)


def make_server(queue, token, host, port):
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            if not self._authorised():
                self._reply(401)
            else:
                url = urllib.parse.urlsplit(self.path)
                status_match = re.fullmatch(r"/jobs/(\d+)", url.path)
                log_match = re.fullmatch(r"/jobs/(\d+)/log", url.path)
                if log_match:
                    offset = int(urllib.parse.parse_qs(url.query).get("from", ["0"])[0])
                    self._reply_text(queue.log(int(log_match.group(1)), offset))
                elif status_match:
                    job_id = int(status_match.group(1))
                    self._reply(200, {"state": queue.state(job_id), "summary": queue.summary(job_id)})
                else:
                    job = queue.next()
                    if job is None:
                        self._reply(204)
                    else:
                        self._reply(200, job)

        def do_POST(self):
            if not self._authorised():
                self._reply(401)
            else:
                body = self.rfile.read(int(self.headers["Content-Length"]))
                log_match = re.fullmatch(r"/jobs/(\d+)/log", self.path)
                result_match = re.fullmatch(r"/jobs/(\d+)/result", self.path)
                if log_match:
                    queue.append_log(int(log_match.group(1)), body.decode())
                    self._reply(204)
                elif result_match:
                    result = json.loads(body)
                    queue.finish(int(result_match.group(1)), result["outcome"], result["summary"])
                    self._reply(204)
                else:
                    request = json.loads(body)
                    job_id = queue.submit(request["type"], request["args"])
                    self._reply(201, {"id": job_id})

        def _authorised(self):
            return hmac.compare_digest(self.headers.get("X-Runner-Token", ""), token)

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

    return http.server.HTTPServer((host, port), Handler)
