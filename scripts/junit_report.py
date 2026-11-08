#!/usr/bin/env python3
"""Read CI's JUnit artifacts into one model of what a run tested.

Every lane that runs tests uploads an artifact named ``junit-<lane>``. A unit,
integration or build lane holds CppUTest files, ``cpputest_<Executable>_<Group>.xml``,
one per test group, each case carrying its source file and line. A BDD lane
holds behave's ``TESTS-<feature>.xml``, one per feature, each case a scenario.
This module reads a directory of those artifacts into lanes of tests, with the
kind of lane, the executable, the suite, the status, the duration, the source
position, a failure's message, and for a scenario its tags, read from the
feature files in the tree so a skip can be explained by what skipped it.

The report pages are rendered from this model by ``junit_report_pages.py``; the
command line that runs both is ``junit_report_main.py``.
"""

import os
import posixpath
import re
import xml.etree.ElementTree as ElementTree
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

ARTIFACT_PREFIX = "junit-"
CPPUTEST_PREFIX = "cpputest_"
BEHAVE_PREFIX = "TESTS-"
FEATURES_DIR = "Bdd/features"
UNNAMED = "unnamed executable"

# Where the checkout sits on a runner, so a case's absolute path becomes the
# repository-relative one. Linux containers, the Windows runner, and anything
# that holds a Tests/ or Bdd/ directory at all.
REPO_ROOT = re.compile(r"^.*?/(?=(Tests|Bdd|Core|Platform)/)")
REGISTRY = re.compile(r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE)
REGISTRY_TOKEN = re.compile(r'"([A-Za-z0-9]+)\|')
OUTLINE_ROW = re.compile(r" -- @\d+\.\d+ .*$")
TAG_LINE = re.compile(r"^\s*(@\S+(?:\s+@\S+)*)\s*$")
SCENARIO_LINE = re.compile(r"^\s*Scenario(?: Outline| Template)?:\s*(.+?)\s*$")
FEATURE_LINE = re.compile(r"^\s*Feature:\s*(.+?)\s*$")


@dataclass(frozen=True)
class Test:
    lane: str
    executable: str
    suite: str
    name: str
    status: str
    time: float
    file: Optional[str] = None
    line: Optional[int] = None
    message: Optional[str] = None
    tags: Tuple[str, ...] = ()

    @property
    def scenario(self) -> str:
        """A scenario outline's row shares its outline's name and tags."""
        return OUTLINE_ROW.sub("", self.name)


@dataclass
class Counts:
    total: int = 0
    passed: int = 0
    failed: int = 0
    skipped: int = 0
    time: float = 0.0

    def add(self, test: Test) -> None:
        self.total += 1
        setattr(self, test.status, getattr(self, test.status) + 1)
        self.time += test.time


@dataclass
class Lane:
    name: str
    tests: List[Test] = field(default_factory=list)

    @property
    def kind(self) -> str:
        return kind_of(self.name)

    @property
    def counts(self) -> Counts:
        return count(self.tests)


@dataclass
class Run:
    lanes: Dict[str, Lane] = field(default_factory=dict)

    @property
    def tests(self) -> List[Test]:
        return [test for lane in self.lanes.values() for test in lane.tests]


def count(tests) -> Counts:
    counts = Counts()
    for test in tests:
        counts.add(test)
    return counts


def kind_of(lane: str) -> str:
    return lane.split("-", 1)[0]


def read_run(artifacts_dir: str, features_dir: Optional[str] = FEATURES_DIR) -> Run:
    """Every ``junit-<lane>`` directory under `artifacts_dir`, as a Run."""
    tags = feature_tags(features_dir) if features_dir and os.path.isdir(features_dir) else {}
    run = Run()
    for entry in sorted(os.listdir(artifacts_dir)):
        if not entry.startswith(ARTIFACT_PREFIX):
            continue
        lane = Lane(entry[len(ARTIFACT_PREFIX) :])
        lane_dir = os.path.join(artifacts_dir, entry)
        for root, _, names in sorted(os.walk(lane_dir)):
            subdirectory = os.path.relpath(root, lane_dir).replace(os.sep, "/")
            for name in sorted(names):
                if name.startswith(CPPUTEST_PREFIX) and name.endswith(".xml"):
                    lane.tests.extend(read_cpputest(os.path.join(root, name), lane.name, subdirectory))
                elif name.startswith(BEHAVE_PREFIX) and name.endswith(".xml"):
                    lane.tests.extend(read_behave(os.path.join(root, name), lane.name, tags))
        run.lanes[lane.name] = lane
    return run


def read_cpputest(path: str, lane: str, subdirectory: str = ".") -> List[Test]:
    tests = []
    for case in ElementTree.parse(path).getroot().iter("testcase"):
        package, _, group = case.get("classname", "").rpartition(".")
        executable = package if package else UNNAMED if subdirectory == "." else f"{UNNAMED} ({subdirectory})"
        status, message = outcome(case)
        line = case.get("line")
        tests.append(
            Test(
                lane=lane,
                executable=executable,
                suite=group,
                name=case.get("name", ""),
                status=status,
                time=float(case.get("time") or 0.0),
                file=repo_relative(case.get("file")),
                line=int(line) if line else None,
                message=message,
            )
        )
    return tests


def read_behave(path: str, lane: str, tags: Dict[Tuple[str, str], Tuple[str, ...]]) -> List[Test]:
    tests = []
    for case in ElementTree.parse(path).getroot().iter("testcase"):
        stem, _, feature = case.get("classname", "").partition(".")
        name = case.get("name", "")
        status, message = outcome(case)
        tests.append(
            Test(
                lane=lane,
                executable="behave",
                suite=feature,
                name=name,
                status=status,
                time=float(case.get("time") or 0.0),
                file=f"{FEATURES_DIR}/{stem}.feature",
                message=message,
                tags=tags.get((stem, OUTLINE_ROW.sub("", name)), tags.get((stem, ""), ())),
            )
        )
    return tests


def outcome(case) -> Tuple[str, Optional[str]]:
    """The case's status and, for a failure, its message."""
    for tag in ("failure", "error"):
        found = case.find(tag)
        if found is not None:
            message = (found.get("message") or "").strip()
            text = (found.text or "").strip()
            return "failed", "\n".join(part for part in (message, text) if part) or tag
    if case.find("skipped") is not None:
        return "skipped", None
    return "passed", None


def repo_relative(path: Optional[str]) -> Optional[str]:
    if not path:
        return None
    return REPO_ROOT.sub("", path.replace("\\", "/"))


def feature_tags(features_dir: str) -> Dict[Tuple[str, str], Tuple[str, ...]]:
    """(feature file stem, scenario name) -> its tags, the feature's included.
    (stem, "") carries the feature's own tags, for a scenario the file no longer
    names."""
    found = {}
    for name in sorted(os.listdir(features_dir)):
        if not name.endswith(".feature"):
            continue
        stem = name[: -len(".feature")]
        with open(os.path.join(features_dir, name), encoding="utf-8") as handle:
            found.update(tags_in(handle.read(), stem))
    return found


def tags_in(text: str, stem: str) -> Dict[Tuple[str, str], Tuple[str, ...]]:
    found = {}
    feature_tags: Tuple[str, ...] = ()
    pending: List[str] = []
    for line in text.splitlines():
        tag_match = TAG_LINE.match(line)
        if tag_match:
            pending.extend(tag.lstrip("@") for tag in tag_match.group(1).split())
            continue
        if FEATURE_LINE.match(line):
            feature_tags = tuple(sorted(pending))
            found[(stem, "")] = feature_tags
            pending = []
            continue
        scenario = SCENARIO_LINE.match(line)
        if scenario:
            found[(stem, scenario.group(1))] = tuple(sorted(set(feature_tags) | set(pending)))
            pending = []
    return found


def registry_tokens(cmake_text: str) -> List[str]:
    """The platform tokens SOLIDSYSLOG_PLATFORM_REGISTRY declares, in order."""
    match = REGISTRY.search(cmake_text)
    return REGISTRY_TOKEN.findall(match.group(1)) if match else []


def pack_of(file: Optional[str], tokens: List[str]) -> str:
    """Which pack a test file exercises: the pack its name carries, else the
    pack its directory is named for, else Core. Longest token first, so
    LwipSocket is not read as Lwip."""
    if not file:
        return "Core"
    if file.startswith(FEATURES_DIR + "/"):
        return "BDD scenarios"
    if file.startswith("Tests/Bdd/"):
        return "BDD target"
    base = posixpath.basename(file)
    ordered = sorted(tokens, key=len, reverse=True)
    for token in ordered:
        if base.startswith("SolidSyslog" + token):
            return token
    for part in file.split("/")[1:-1]:
        for token in ordered:
            if part.startswith(token):
                return token
    if base.endswith("FakeTest.cpp"):
        return "Test support"
    return "Core"
