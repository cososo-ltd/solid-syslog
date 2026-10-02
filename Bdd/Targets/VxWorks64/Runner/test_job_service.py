"""Tests for the VxWorks 6.4 runner's job service (job_service.py).

Run:  python -m unittest discover -s Bdd/Targets/VxWorks64/Runner -p 'test_*.py'
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import job_service  # noqa: E402


class JobQueueTest(unittest.TestCase):
    def test_new_queue_has_no_next_job(self):
        queue = job_service.JobQueue()
        self.assertIsNone(queue.next())

    def test_submitted_job_is_next(self):
        queue = job_service.JobQueue()
        job_id = queue.submit("build", {"clean": True})
        self.assertEqual({"id": job_id, "type": "build", "args": {"clean": True}}, queue.next())

    def test_jobs_are_taken_once_each_in_order(self):
        queue = job_service.JobQueue()
        first = queue.submit("checkout", {"ref": "main"})
        second = queue.submit("build", {})
        self.assertNotEqual(first, second)
        self.assertEqual(first, queue.next()["id"])
        self.assertEqual(second, queue.next()["id"])
        self.assertIsNone(queue.next())

    def test_submitted_job_is_queued(self):
        queue = job_service.JobQueue()
        job_id = queue.submit("build", {})
        self.assertEqual("queued", queue.state(job_id))


if __name__ == "__main__":
    unittest.main()
