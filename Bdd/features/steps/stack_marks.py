"""Recording the stack high-water marks an RTOS target prints as it quits.

The QEMU targets end a scenario with one line giving the least stack each task
ever had left, in bytes:

    [stack-hwm] interactive=1234 bytes service=5678 bytes

The interactive task is the one that logs; the service task runs Service. A
target boots once per scenario, so each scenario gives one pair, and the run's
figure for a task is the least any scenario left it. This is the one figure
only a target can give, so the run keeps it: every pair goes to a file beside
the JUnit results and the least of each is printed when the run ends.
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
            # A scenario whose Service task never started reports zero for it,
            # which is no measurement.
            started = [mark for mark in self._marks if mark[2] > 0]
            lines.append(
                f"[stack-hwm] least interactive={least_interactive[1]} bytes in: {least_interactive[0]}"
            )
            if started:
                least_service = min(started, key=lambda mark: mark[2])
                lines.append(f"[stack-hwm] least service={least_service[2]} bytes in: {least_service[0]}")
        return lines

    def write(self, path=STACK_MARKS_PATH):
        """Writes the record where the run's results go and returns its lines; nothing for a run with no marks."""
        lines = self.lines()
        if lines:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8") as record:
                record.write("\n".join(lines) + "\n")
        return lines
