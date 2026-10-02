"""Tests for the VxWorks 6.4 runner's job service (job_service.py).

Run:  python -m unittest discover -s Bdd/Targets/VxWorks64/Runner -p 'test_*.py'
"""

import json
import os
import sys
import threading
import unittest
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import job_service  # noqa: E402


class JobQueueTest(unittest.TestCase):
    def setUp(self):
        self.queue = job_service.JobQueue()

    def running_job(self):
        job_id = self.queue.submit("build", {})
        self.queue.next()
        return job_id

    def test_new_queue_has_no_next_job(self):
        self.assertIsNone(self.queue.next())

    def test_submitted_job_is_next(self):
        job_id = self.queue.submit("build", {"clean": True})
        self.assertEqual({"id": job_id, "type": "build", "args": {"clean": True}}, self.queue.next())

    def test_jobs_are_taken_once_each_in_order(self):
        first = self.queue.submit("checkout", {"ref": "main"})
        second = self.queue.submit("build", {})
        self.assertNotEqual(first, second)
        self.assertEqual(first, self.queue.next()["id"])
        self.assertEqual(second, self.queue.next()["id"])
        self.assertIsNone(self.queue.next())

    def test_submitted_job_is_queued(self):
        job_id = self.queue.submit("build", {})
        self.assertEqual("queued", self.queue.state(job_id))

    def test_taken_job_is_running(self):
        job_id = self.running_job()
        self.assertEqual("running", self.queue.state(job_id))

    def test_finished_job_reports_its_outcome(self):
        job_id = self.running_job()
        self.queue.finish(job_id, "failed", "Diagnostics: 1")
        self.assertEqual("failed", self.queue.state(job_id))

    def test_finished_job_keeps_its_summary(self):
        job_id = self.running_job()
        self.queue.finish(job_id, "failed", "Diagnostics: 1")
        self.assertEqual("Diagnostics: 1", self.queue.summary(job_id))

    def test_log_reads_back_the_chunks_in_order(self):
        job_id = self.running_job()
        self.queue.append_log(job_id, "first\n")
        self.queue.append_log(job_id, "second\n")
        self.assertEqual("first\nsecond\n", self.queue.log(job_id, 0))

    def test_log_from_an_offset_returns_what_follows(self):
        job_id = self.running_job()
        self.queue.append_log(job_id, "first\n")
        self.queue.append_log(job_id, "second\n")
        self.assertEqual("second\n", self.queue.log(job_id, len("first\n")))

    def test_log_of_a_job_with_no_output_is_empty(self):
        job_id = self.queue.submit("build", {})
        self.assertEqual("", self.queue.log(job_id, 0))


class JobServiceTest(unittest.TestCase):
    TOKEN = "test-token"

    def setUp(self):
        self.queue = job_service.JobQueue()
        self.server = job_service.make_server(self.queue, self.TOKEN, "127.0.0.1", 0)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.thread.join()
        self.server.server_close()

    def request(self, method, path, token=None, body=None):
        port = self.server.server_address[1]
        if isinstance(body, str):
            data = body.encode()
        elif body is None:
            data = None
        else:
            data = json.dumps(body).encode()
        request = urllib.request.Request(f"http://127.0.0.1:{port}{path}", data=data, method=method)
        if token is not None:
            request.add_header("X-Runner-Token", token)
        try:
            with urllib.request.urlopen(request) as response:
                return response.status, response.read()
        except urllib.error.HTTPError as error:
            return error.code, error.read()

    def test_request_without_the_token_is_refused(self):
        status, _ = self.request("GET", "/jobs/next")
        self.assertEqual(401, status)

    def test_request_with_the_wrong_token_is_refused(self):
        status, _ = self.request("GET", "/jobs/next", "not-the-token")
        self.assertEqual(401, status)

    def test_next_with_no_job_waiting_is_no_content(self):
        status, _ = self.request("GET", "/jobs/next", self.TOKEN)
        self.assertEqual(204, status)

    def test_job_posted_without_the_token_is_refused_and_not_queued(self):
        status, _ = self.request("POST", "/jobs", None, {"type": "build", "args": {}})
        self.assertEqual(401, status)
        self.assertIsNone(self.queue.next())

    def test_posted_job_answers_its_id(self):
        status, body = self.request("POST", "/jobs", self.TOKEN, {"type": "build", "args": {}})
        self.assertEqual(201, status)
        self.assertEqual({"id": 1}, json.loads(body))

    def test_next_hands_out_a_queued_job(self):
        job_id = self.queue.submit("checkout", {"ref": "main"})
        status, body = self.request("GET", "/jobs/next", self.TOKEN)
        self.assertEqual(200, status)
        self.assertEqual({"id": job_id, "type": "checkout", "args": {"ref": "main"}}, json.loads(body))

    def test_posted_log_text_is_appended_to_the_job(self):
        job_id = self.queue.submit("build", {})
        self.queue.next()
        status, _ = self.request("POST", f"/jobs/{job_id}/log", self.TOKEN, "building\n")
        self.assertEqual(204, status)
        self.assertEqual("building\n", self.queue.log(job_id, 0))

    def test_posted_result_finishes_the_job(self):
        job_id = self.queue.submit("build", {})
        self.queue.next()
        result = {"outcome": "succeeded", "summary": "Diagnostics: none"}
        status, _ = self.request("POST", f"/jobs/{job_id}/result", self.TOKEN, result)
        self.assertEqual(204, status)
        self.assertEqual("succeeded", self.queue.state(job_id))
        self.assertEqual("Diagnostics: none", self.queue.summary(job_id))

    def test_status_of_a_queued_job_has_no_summary(self):
        job_id = self.queue.submit("build", {})
        status, body = self.request("GET", f"/jobs/{job_id}", self.TOKEN)
        self.assertEqual(200, status)
        self.assertEqual({"state": "queued", "summary": None}, json.loads(body))

    def test_log_is_read_back_from_an_offset(self):
        job_id = self.queue.submit("build", {})
        self.queue.append_log(job_id, "first\nsecond\n")
        status, body = self.request("GET", f"/jobs/{job_id}/log?from=6", self.TOKEN)
        self.assertEqual(200, status)
        self.assertEqual("second\n", body.decode())

    def test_get_of_an_unknown_path_is_not_found_and_takes_no_job(self):
        job_id = self.queue.submit("build", {})
        status, _ = self.request("GET", "/elsewhere", self.TOKEN)
        self.assertEqual(404, status)
        self.assertEqual("queued", self.queue.state(job_id))

    def test_status_of_an_unknown_job_is_not_found(self):
        status, _ = self.request("GET", "/jobs/99", self.TOKEN)
        self.assertEqual(404, status)

    def test_log_of_an_unknown_job_is_not_found(self):
        status, _ = self.request("GET", "/jobs/99/log", self.TOKEN)
        self.assertEqual(404, status)

    def test_result_for_an_unknown_job_is_not_found(self):
        result = {"outcome": "succeeded", "summary": ""}
        status, _ = self.request("POST", "/jobs/99/result", self.TOKEN, result)
        self.assertEqual(404, status)
        self.assertFalse(self.queue.knows(99))

    def test_log_for_an_unknown_job_is_not_found(self):
        status, _ = self.request("POST", "/jobs/99/log", self.TOKEN, "stray\n")
        self.assertEqual(404, status)
        self.assertEqual("", self.queue.log(99, 0))

    def test_post_to_an_unknown_path_is_not_found_and_queues_nothing(self):
        status, _ = self.request("POST", "/elsewhere", self.TOKEN, {"type": "build", "args": {}})
        self.assertEqual(404, status)
        self.assertIsNone(self.queue.next())


if __name__ == "__main__":
    unittest.main()
