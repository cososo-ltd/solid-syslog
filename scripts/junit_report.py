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

The report pages are rendered from this model by ``junit_report_pages.py``,
which is also the command line that runs both.
"""

import json
import os
import posixpath
import re
import xml.etree.ElementTree as ElementTree
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

ARTIFACT_PREFIX = "junit-"
CPPUTEST_PREFIX = "cpputest_"
BEHAVE_PREFIX = "TESTS-"
VERSIONS_FILE = "solidsyslog-versions.json"
SHORT_COMMIT = 7
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
KEYWORD_LINE = re.compile(r"^\s*(Background|Rule|Scenario|Scenario Outline|Scenario Template|Examples):")
# One step as behave's JUnit output records it, under the scenario heading in
# the case's system-out: keyword, text, outcome and duration.
STEP_LINE = re.compile(r"^\s*(Given|When|Then|And|But|\*)\s+(.*?)\s+\.\.\.\s+(passed|failed|skipped|undefined|untested|error)(?:\s+in\s+([0-9.]+)s)?\s*$")


@dataclass(frozen=True)
class Step:
    keyword: str
    text: str
    status: str
    time: float


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
    steps: Tuple[Step, ...] = ()

    @property
    def feature_stem(self) -> Optional[str]:
        """The feature file's stem, for a scenario; None for anything else."""
        if self.file and self.file.startswith(FEATURES_DIR + "/"):
            return self.file[len(FEATURES_DIR) + 1 : -len(".feature")]
        return None

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


@dataclass(frozen=True)
class Upstream:
    """One upstream tree a pack compiled against, as its header and its git
    checkout described it. The version is what the header states; the tag and
    commit are what the checkout is. Any of the three may be missing."""

    tree: str
    version: Optional[str] = None
    tag: Optional[str] = None
    commit: Optional[str] = None
    path: Optional[str] = None

    @property
    def label(self) -> str:
        if self.version:
            return self.version
        if self.tag:
            return self.tag
        return (self.commit or "")[:SHORT_COMMIT]

    def answers_to(self, claim: str) -> bool:
        """Whether a page's claim names this tree: its version, its tag with
        or without a leading v, or its commit by at least its short form."""
        if not claim:
            return False
        if claim in (self.version, self.tag):
            return True
        if self.tag and self.tag.startswith("v") and claim == self.tag[1:]:
            return True
        return bool(self.commit) and len(claim) >= SHORT_COMMIT and self.commit.startswith(claim)


@dataclass(frozen=True)
class Versions:
    """What a lane's configure compiled against, from the record CMake writes
    beside the build (cmake/VersionRecord.cmake)."""

    compiler_id: str
    compiler_version: str
    compiler_executable: str
    system: str
    cross_compiling: bool
    packs: Tuple[str, ...]
    bdd_target: Optional[str]
    upstreams: Dict[str, Tuple[Upstream, ...]]

    @property
    def compiler(self) -> str:
        return f"{self.compiler_id} {self.compiler_version}"


@dataclass
class Lane:
    name: str
    tests: List[Test] = field(default_factory=list)
    versions: Optional[Versions] = None

    @property
    def kind(self) -> str:
        return kind_of(self.name)

    @property
    def counts(self) -> Counts:
        return count(self.tests)


@dataclass
class Feature:
    """What a feature file says about itself: its name, the prose under the
    Feature line, and the tags of the feature and of each scenario."""

    stem: str
    name: str = ""
    description: str = ""
    feature_tags: Tuple[str, ...] = ()
    tags: Dict[str, Tuple[str, ...]] = field(default_factory=dict)

    def keys(self):
        return [(self.stem, scenario) for scenario in self.tags]


@dataclass
class Run:
    lanes: Dict[str, Lane] = field(default_factory=dict)
    features: Dict[str, Feature] = field(default_factory=dict)

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
    features = read_features(features_dir) if features_dir and os.path.isdir(features_dir) else {}
    tags = {key: feature.tags[key[1]] for feature in features.values() for key in feature.keys()}
    run = Run(features=features)
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
                elif name == VERSIONS_FILE:
                    lane.versions = read_versions(os.path.join(root, name))
        run.lanes[lane.name] = lane
    return run


def read_versions(path: str) -> Versions:
    with open(path, encoding="utf-8") as handle:
        record = json.load(handle)
    compiler = record.get("compiler", {})
    system = record.get("system", {})
    upstreams = {
        pack: tuple(
            Upstream(entry["tree"], entry.get("version"), entry.get("tag"), entry.get("commit"), entry.get("path"))
            for entry in entries
        )
        for pack, entries in record.get("upstreams", {}).items()
    }
    return Versions(
        compiler.get("id", ""),
        compiler.get("version", ""),
        compiler.get("executable", ""),
        " ".join(part for part in (system.get("name"), system.get("processor")) if part),
        bool(system.get("crossCompiling", False)),
        tuple(record.get("packs", [])),
        record.get("bddTarget"),
        upstreams,
    )


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
                steps=steps_in(case.findtext("system-out") or ""),
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


def steps_in(system_out: str) -> Tuple[Step, ...]:
    """The scenario's steps as behave recorded them in the case's output."""
    steps = []
    for line in system_out.splitlines():
        match = STEP_LINE.match(line)
        if match:
            keyword, text, status, time = match.groups()
            steps.append(Step(keyword, text, status, float(time) if time else 0.0))
    return tuple(steps)


def repo_relative(path: Optional[str]) -> Optional[str]:
    if not path:
        return None
    return REPO_ROOT.sub("", path.replace("\\", "/"))


def read_features(features_dir: str) -> Dict[str, Feature]:
    """Every feature file in the directory, by stem."""
    found = {}
    for name in sorted(os.listdir(features_dir)):
        if not name.endswith(".feature"):
            continue
        stem = name[: -len(".feature")]
        with open(os.path.join(features_dir, name), encoding="utf-8") as handle:
            found[stem] = parse_feature(handle.read(), stem)
    return found


def parse_feature(text: str, stem: str) -> Feature:
    feature = Feature(stem)
    pending: List[str] = []
    in_description = False
    description: List[str] = []
    for line in text.splitlines():
        tag_match = TAG_LINE.match(line)
        if tag_match:
            in_description = False
            pending.extend(tag.lstrip("@") for tag in tag_match.group(1).split())
            continue
        title = FEATURE_LINE.match(line)
        if title:
            feature.name = title.group(1)
            feature.feature_tags = tuple(sorted(pending))
            feature.tags[""] = feature.feature_tags
            pending = []
            in_description = True
            continue
        if KEYWORD_LINE.match(line):
            in_description = False
        scenario = SCENARIO_LINE.match(line)
        if scenario:
            feature.tags[scenario.group(1)] = tuple(sorted(set(feature.feature_tags) | set(pending)))
            pending = []
            continue
        if in_description and line.strip():
            description.append(line.strip())
    feature.description = "\n".join(description)
    return feature


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
