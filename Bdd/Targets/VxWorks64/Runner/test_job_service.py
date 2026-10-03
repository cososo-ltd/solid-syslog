"""Tests for the VxWorks 6.4 runner's job service (job_service.py).

Run:  python -m unittest discover -s Bdd/Targets/VxWorks64/Runner -p 'test_*.py'
"""

import io
import json
import os
import shutil
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import time
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


class BodyLimitTest(unittest.TestCase):
    def test_a_body_over_the_limit_is_refused_and_nothing_queued(self):
        queue = job_service.JobQueue()
        server = job_service.make_server(queue, "test-token", "127.0.0.1", 0, max_body_bytes=16)
        thread = threading.Thread(target=server.serve_forever)
        thread.start()
        try:
            body = json.dumps({"type": "build", "args": {"padding": "x" * 64}}).encode()
            request = urllib.request.Request(f"http://127.0.0.1:{server.server_address[1]}/jobs",
                                             data=body, method="POST")
            request.add_header("X-Runner-Token", "test-token")
            # Refusing without reading leaves the body unread, so the client may
            # see the connection close rather than the 413.
            try:
                urllib.request.urlopen(request).close()
                refused = False
            except urllib.error.HTTPError as error:
                refused = error.code == 413
            except (ConnectionError, urllib.error.URLError):
                refused = True
            self.assertTrue(refused)
            self.assertIsNone(queue.next())
        finally:
            server.shutdown()
            thread.join()
            server.server_close()


class ConsoleTest(unittest.TestCase):
    def test_what_the_target_sends_is_written_out_until_it_disconnects(self):
        listener = socket.create_server(("127.0.0.1", 0))
        out = io.BytesIO()
        relay = threading.Thread(target=job_service.relay_console, args=(listener, out))
        relay.start()
        with socket.create_connection(listener.getsockname()) as target:
            target.sendall(b"SolidSyslog VxWorks 6.4 BDD target: Core ran\r\n")
        relay.join(5)
        listener.close()
        self.assertEqual(b"SolidSyslog VxWorks 6.4 BDD target: Core ran\r\n", out.getvalue())

    # Stopping QEMU kills it, so its end of the console is reset, not closed.
    def test_a_reset_connection_ends_the_relay_and_keeps_what_arrived(self):
        listener = socket.create_server(("127.0.0.1", 0))
        out = io.BytesIO()
        failures = []

        def relay():
            try:
                job_service.relay_console(listener, out)
            except OSError as error:
                failures.append(error)

        thread = threading.Thread(target=relay)
        thread.start()
        target = socket.create_connection(listener.getsockname())
        target.sendall(b"booting\r\n")
        time.sleep(0.2)
        target.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        target.close()
        thread.join(5)
        listener.close()
        self.assertEqual([], failures)
        self.assertEqual(b"booting\r\n", out.getvalue())


class CommandLineTest(unittest.TestCase):
    def test_name_value_words_become_job_arguments(self):
        self.assertEqual({"ref": "main", "tool": "sfdiab"}, job_service.job_arguments(["ref=main", "tool=sfdiab"]))


# Stands in for the runner: takes the next job, logs, and finishes it.
def finish_next_job(queue, log, outcome, summary):
    deadline = time.monotonic() + 5
    job = None
    while (job is None) and (time.monotonic() < deadline):
        job = queue.next()
        time.sleep(0.01)
    if job is not None:
        queue.append_log(job["id"], log)
        queue.finish(job["id"], outcome, summary)


class RunJobTest(unittest.TestCase):
    TOKEN = "test-token"

    def setUp(self):
        self.queue = job_service.JobQueue()
        self.server = job_service.make_server(self.queue, self.TOKEN, "127.0.0.1", 0)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.start()
        self.url = f"http://127.0.0.1:{self.server.server_address[1]}"

    def tearDown(self):
        self.server.shutdown()
        self.thread.join()
        self.server.server_close()

    def runner_finishes_next_job(self, log, outcome, summary):
        thread = threading.Thread(target=finish_next_job, args=(self.queue, log, outcome, summary), daemon=True)
        thread.start()
        return thread

    def test_run_job_returns_the_outcome_and_summary(self):
        runner = self.runner_finishes_next_job("building\n", "succeeded", "Diagnostics: none")
        result = job_service.run_job(self.url, self.TOKEN, "build", {}, io.StringIO(), poll_seconds=0.01)
        runner.join()
        self.assertEqual(("succeeded", "Diagnostics: none"), result)

    def test_run_job_writes_the_jobs_log(self):
        out = io.StringIO()
        runner = self.runner_finishes_next_job("building\nbuilt\n", "succeeded", "")
        job_service.run_job(self.url, self.TOKEN, "build", {}, out, poll_seconds=0.01)
        runner.join()
        self.assertEqual("building\nbuilt\n", out.getvalue())


OPENSSL = shutil.which("openssl")


def make_certificate(directory):
    certificate = os.path.join(directory, "certificate.pem")
    key = os.path.join(directory, "key.pem")
    subprocess.run(
        [OPENSSL, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", key,
         "-out", certificate, "-days", "1", "-subj", "/CN=solidsyslog-runner-test"],
        check=True, capture_output=True)
    return certificate, key


@unittest.skipUnless(OPENSSL, "openssl is needed to make a test certificate")
class CertificateTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.certificate, self.key = make_certificate(self.directory)

    def tearDown(self):
        shutil.rmtree(self.directory)

    def test_thumbprint_is_the_certificates_sha1_in_upper_case_hex(self):
        fingerprint = subprocess.run(
            [OPENSSL, "x509", "-in", self.certificate, "-noout", "-fingerprint", "-sha1"],
            check=True, capture_output=True, text=True).stdout
        expected = fingerprint.strip().split("=", 1)[1].replace(":", "")
        self.assertEqual(expected, job_service.thumbprint(self.certificate))

    def test_initialise_creates_the_token_certificate_and_key(self):
        home = os.path.join(self.directory, "home")
        job_service.initialise(home)
        with open(os.path.join(home, "token"), encoding="ascii") as token:
            self.assertGreaterEqual(len(token.read().strip()), 32)
        self.assertEqual(40, len(job_service.thumbprint(os.path.join(home, "certificate.pem"))))
        self.assertTrue(os.path.isfile(os.path.join(home, "key.pem")))

    def test_initialise_refuses_to_replace_an_existing_setup(self):
        home = os.path.join(self.directory, "home")
        job_service.initialise(home)
        with open(os.path.join(home, "token"), encoding="ascii") as token:
            original = token.read()
        with self.assertRaises(FileExistsError):
            job_service.initialise(home)
        with open(os.path.join(home, "token"), encoding="ascii") as token:
            self.assertEqual(original, token.read())

    def test_served_with_a_certificate_the_service_answers_over_tls(self):
        server = job_service.make_server(job_service.JobQueue(), "test-token", "127.0.0.1", 0,
                                         (self.certificate, self.key))
        thread = threading.Thread(target=server.serve_forever)
        thread.start()
        try:
            client = ssl.create_default_context()
            client.check_hostname = False
            client.verify_mode = ssl.CERT_NONE
            request = urllib.request.Request(f"https://127.0.0.1:{server.server_address[1]}/jobs/next")
            request.add_header("X-Runner-Token", "test-token")
            with urllib.request.urlopen(request, context=client) as response:
                self.assertEqual(204, response.status)
        finally:
            server.shutdown()
            thread.join()
            server.server_close()

    def test_run_job_verifies_the_service_against_its_certificate(self):
        queue = job_service.JobQueue()
        server = job_service.make_server(queue, "test-token", "127.0.0.1", 0, (self.certificate, self.key))
        thread = threading.Thread(target=server.serve_forever)
        thread.start()
        queue_runner = threading.Thread(target=finish_next_job, args=(queue, "", "succeeded", "done"), daemon=True)
        queue_runner.start()
        try:
            client = ssl.create_default_context(cafile=self.certificate)
            client.check_hostname = False
            result = job_service.run_job(f"https://127.0.0.1:{server.server_address[1]}", "test-token",
                                         "build", {}, io.StringIO(), poll_seconds=0.01, context=client)
            self.assertEqual(("succeeded", "done"), result)
        finally:
            queue_runner.join()
            server.shutdown()
            thread.join()
            server.server_close()


if __name__ == "__main__":
    unittest.main()
