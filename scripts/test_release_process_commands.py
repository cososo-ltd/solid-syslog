"""Checks for the release-process command examples.

Run: python3 scripts/test_release_process_commands.py

These tests parse the documented commands and check their arguments; they do
not contact GitHub or assert GitHub's release and upload behaviour.
"""

import re
import shlex
import subprocess
import unittest
from pathlib import Path


DOCUMENT = Path(__file__).resolve().parents[1] / "docs" / "release-process.md"


def section(document, heading):
    return document.split(heading + "\n", 1)[1].split("\n##", 1)[0]


def substitute(command, values):
    for name, value in values.items():
        command = command.replace(f"<{name}>", shlex.quote(value))
    return command


def arguments(command):
    # Bash consumes a backslash followed by a newline before splitting words.
    return shlex.split(command.replace("\\\n", ""))


class ExampleStackReleaseCommand(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = section(DOCUMENT.read_text(encoding="utf-8"),
                       "### Releasing the example stacks")
        cls.command = re.search(r"```bash\n(.*?)\n\s*```", text, re.DOTALL).group(1)

    def test_release_command_targets_each_stack_and_attaches_the_matching_sweep(self):
        for stack in ("solid-syslog-example", "solid-syslog-example-make"):
            for version in ("0.4.0", "1.12.34"):
                with self.subTest(stack=stack, version=version):
                    command = substitute(self.command, {
                        "stack": stack,
                        "version": version,
                        "tip": "abcdef123456",
                        "file": "release notes.md",
                    })
                    syntax = subprocess.run(["bash", "-n"], input=command,
                                            text=True, capture_output=True, check=False)
                    self.assertEqual(0, syntax.returncode, syntax.stderr)
                    argv = arguments(command)
                    self.assertEqual(["gh", "release", "create", f"v{version}"], argv[:4])
                    options = {}
                    positional = []
                    words = iter(argv[4:])
                    for word in words:
                        if word in ("--repo", "--target", "--title", "--notes-file"):
                            self.assertNotIn(word, options)
                            options[word] = next(words)
                        elif word == "--latest":
                            self.assertNotIn(word, options)
                            options[word] = True
                        else:
                            positional.append(word)
                    self.assertEqual({
                        "--repo": f"cososo-ltd/{stack}",
                        "--target": "abcdef123456",
                        "--title": version,
                        "--notes-file": "release notes.md",
                        "--latest": True,
                    }, options)
                    # A missing or mismatched asset would publish an incomplete release.
                    self.assertEqual([f"sweep-{version}.zip"], positional)


class MilestoneCommands(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        checklist = section(DOCUMENT.read_text(encoding="utf-8"), "## Checklist")
        milestone = checklist.split("- [ ] Close the release's milestone", 1)[1]
        milestone = milestone.split("\n- [ ]", 1)[0]
        cls.commands = re.findall(r"`(gh api.*?)`", milestone, re.DOTALL)

    def test_close_create_and_list_use_the_library_repository(self):
        self.assertEqual(3, len(self.commands))
        commands = [arguments(substitute(command, {"number": "42", "version": "0.5.0"}))
                    for command in self.commands]
        self.assertEqual([
            ["gh", "api", "-X", "PATCH", "repos/cososo-ltd/solid-syslog/milestones/42",
             "-f", "state=closed"],
            ["gh", "api", "-X", "POST", "repos/cososo-ltd/solid-syslog/milestones",
             "-f", "title=0.5.0"],
            ["gh", "api", "repos/cososo-ltd/solid-syslog/milestones", "--jq", ".[].title"],
        ], commands)


if __name__ == "__main__":
    unittest.main()
