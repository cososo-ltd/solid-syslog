#!/usr/bin/env python3
"""Job service for the VxWorks 6.4 runner.

Runs on the development machine. Jobs are queued here and the runner on the
build machine collects them, one at a time, connecting outwards only.
"""


class JobQueue:
    def __init__(self):
        self._pending = []
        self._states = {}
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

    def finish(self, job_id, outcome, summary):
        self._states[job_id] = outcome

    def state(self, job_id):
        return self._states[job_id]
