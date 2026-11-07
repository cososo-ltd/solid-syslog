"""Recording the stack high-water marks an RTOS target prints as it quits.

The QEMU targets answer `quit` with one line giving the least stack each task
ever had left, in bytes:

    [stack-hwm] interactive=1234 bytes service=5678 bytes

The interactive task is the one that logs; the service task runs Service. A
target boots once per scenario, so a scenario that ends through `quit` gives
one pair, and the run's figure for a task is the least any of them left it. A
scenario that shuts the target down or kills it instead, to stand in for a
power cut, prints none; nor does a boot a scenario replaces by restarting the
target. This is the one figure only a target can give, so the run keeps it:
every pair goes to a file beside the JUnit results and the least of each is
printed when the run ends. Zero is a reading like any other: a task that used
its whole stack reports it, and so does one that never ran.
"""

import os
import re

_MARK = re.compile(rb"\[stack-hwm\] interactive=(\d+) bytes service=(\d+) bytes")

STACK_MARKS_PATH = os.path.join("Bdd", "junit", "stack-hwm.txt")


def stack_marks_in(output):
    """The (interactive, service) headroom in a target's output, or None if it printed none."""
    marks = _MARK.findall(bytes(output))
    if not marks:
        return None
    interactive, service = marks[-1]
    return int(interactive), int(service)


class StackMarks:
    """The marks of one run, scenario by scenario."""

    def __init__(self):
        self._marks = []

    @staticmethod
    def discard_record(path=STACK_MARKS_PATH):
        """Removes an earlier run's record, so the file never describes a run other than the last."""
        try:
            os.remove(path)
        except FileNotFoundError:
            pass

    def record(self, scenario, marks):
        self._marks.append((scenario, marks[0], marks[1]))

    def lines(self):
        """The run's record: one line per scenario, then the least each task was left."""
        lines = [
            f"[stack-hwm] interactive={interactive} service={service} scenario={scenario}"
            for scenario, interactive, service in self._marks
        ]
        if self._marks:
            least_interactive = min(self._marks, key=lambda mark: mark[1])
            least_service = min(self._marks, key=lambda mark: mark[2])
            lines.append(
                f"[stack-hwm] least interactive={least_interactive[1]} bytes in: {least_interactive[0]}"
            )
            lines.append(f"[stack-hwm] least service={least_service[2]} bytes in: {least_service[0]}")
        return lines

    def write(self, path=STACK_MARKS_PATH):
        """Writes the record where the run's results go; nothing for a run with no marks."""
        lines = self.lines()
        if lines:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8") as record:
                record.write("\n".join(lines) + "\n")
