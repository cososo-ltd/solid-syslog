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


if __name__ == "__main__":
    unittest.main()
