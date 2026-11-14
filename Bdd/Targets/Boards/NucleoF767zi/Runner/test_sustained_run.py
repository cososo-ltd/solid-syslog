"""Tests for what a sustained run makes of the collector's record (sustained_run.py).

Run:  python -m unittest discover -s Bdd/Targets/Boards/NucleoF767zi/Runner -p 'test_*.py'
"""

import json
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sustained_run  # noqa: E402


def message(sequence_id, text="Hello", up_time=None):
    return f'<134>1 - host app - id [meta sequenceId="{sequence_id}" sysUpTime="{up_time or sequence_id * 10}"] {text}'


def counted(body):
    return f"{len(body.encode('utf-8'))} {body}"


def line(*bodies):
    records = [{"body": {"stringValue": body}} for body in bodies]
    return json.dumps({"resourceLogs": [{"scopeLogs": [{"logRecords": records}]}]})


class MessagesTest(unittest.TestCase):
    def test_no_lines_hold_no_messages(self):
        self.assertEqual([], sustained_run.messages_in([]))

    def test_a_line_holds_the_message_of_its_record(self):
        self.assertEqual(["one"], sustained_run.messages_in([line("one")]))

    # The exporter writes the records that arrived together as one line.
    def test_a_line_holds_the_message_of_each_of_its_records(self):
        self.assertEqual(["one", "two", "three"], sustained_run.messages_in([line("one", "two"), line("three")]))

    def test_an_empty_line_holds_nothing(self):
        self.assertEqual(["one"], sustained_run.messages_in(["\n", line("one")]))


class FaultsTest(unittest.TestCase):
    def test_every_message_once_and_in_order_has_no_fault(self):
        self.assertEqual([], sustained_run.faults_in([message(1), message(2), message(3)], 1, 3))

    def test_the_first_message_is_the_one_it_is_told(self):
        self.assertEqual([], sustained_run.faults_in([message(4), message(5)], 4, 2))

    def test_a_message_that_did_not_arrive_is_a_fault(self):
        faults = sustained_run.faults_in([message(1), message(3)], 1, 3)
        self.assertEqual(1, len(faults))
        self.assertRegex(faults[0], r"2 messages for the 3 sent, 1 of them missing \(the first: \[2\]\), and the rest in order")

    def test_no_messages_at_all_is_a_fault(self):
        self.assertRegex(sustained_run.faults_in([], 1, 2)[0], "0 messages for the 2 sent, 2 of them missing")

    def test_a_message_that_arrived_twice_is_a_fault(self):
        self.assertRegex(sustained_run.faults_in([message(1), message(1), message(2)], 1, 2)[0], "with repeats")

    def test_messages_out_of_order_are_a_fault(self):
        self.assertRegex(sustained_run.faults_in([message(2), message(1)], 1, 2)[0], "out of order")

    def test_a_message_from_outside_the_run_is_a_fault(self):
        self.assertRegex(sustained_run.faults_in([message(1), message(2), message(9)], 1, 2)[0], "with others among them")

    def test_a_message_with_no_sequence_id_is_a_fault(self):
        faults = sustained_run.faults_in([message(1), "<134>1 - host app - id - Hello"], 1, 2)
        self.assertTrue(any("unlike the first" in fault for fault in faults))
        self.assertTrue(any("1 of them missing" in fault for fault in faults))

    def test_a_message_whose_text_changed_is_a_fault(self):
        faults = sustained_run.faults_in([message(1), message(2, "Hellp")], 1, 2)
        self.assertEqual(1, len(faults))
        self.assertRegex(faults[0], "a message unlike the first: .*Hellp")

    # The up-time counts in milliseconds, and no two runs give the same.
    def test_the_up_time_may_differ(self):
        self.assertEqual([], sustained_run.faults_in([message(1, up_time=5), message(2, up_time=977)], 1, 2))

    def test_a_message_behind_its_octet_count_has_no_fault(self):
        self.assertEqual([], sustained_run.faults_in([counted(message(9)), counted(message(10))], 9, 2))

    # The byte order mark and anything else outside ASCII is several octets.
    def test_the_octet_count_is_of_octets(self):
        self.assertEqual([], sustained_run.faults_in([counted(message(1, "﻿Hello"))], 1, 1))

    def test_an_octet_count_that_is_not_the_length_is_a_fault(self):
        faults = sustained_run.faults_in([f"7 {message(1)}"], 1, 1)
        self.assertEqual(1, len(faults))
        self.assertRegex(faults[0], "an octet count of 7")


if __name__ == "__main__":
    unittest.main()
