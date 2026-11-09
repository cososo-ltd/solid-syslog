#!/usr/bin/env python3
"""Assert every file a cross build read came from the tree it was pinned to.

The cross image carries more than one copy of several upstream files.
CMSIS-FreeRTOS bundles its own FreeRTOS kernel, of a different version from the
one at `SOLIDSYSLOG_FREERTOS_KERNEL_PATH`, and there are example and test
copies of `FreeRTOSConfig.h` and `lwipopts.h` in most of the trees. A change
to an include path or a source list could pick one of them up and still build,
and nothing would say so.

The build already records what every compile read: CMake asks the compiler
for a dependency file per object, so when the build is done the build tree
holds, for each object, the source it was compiled from and every header the
compiler resolved. This reads those rather than re-running the compiles, so it
reports what was actually built. Make leaves the files in place; Ninja folds
them into its own log, which `ninja -t deps` prints back. A source or header
that CMake copied into the build tree at configure time is classified at the
place CMake copied it from, which CMake records as well.

The configure also records which upstream trees the target wires. Every BDD
target resolves its trees through `solidsyslog_bdd_upstream()` in
`Bdd/Targets/Common/UpstreamTrees.cmake`, which leaves their variable names in
`SolidSyslogUpstreamTrees.txt` at the build root. The image pins every tree for
every build, so that list, not the cache, says which trees this target may
read.

Four rules, over every file read:

- **It resolves inside the allowed set.** The allowed set is this checkout,
  the build tree, the compiler's and CMake's own directories, and for each
  tree the target wires, the specific subtrees it wires rather than the whole
  repository. CMSIS-FreeRTOS is allowed only at `CMSIS/RTOS2/FreeRTOS/Source`
  and `CMSIS/RTOS2/FreeRTOS/Include`, so the kernel it bundles under `Source/`
  is outside the set by construction. A pinned tree the target does not wire
  is outside the set entirely.
- **Every tree the target wires was read from.** A wired tree nothing was
  read from is a declaration that has outlived its wiring.
- **A name the pinned kernel owns resolves only in the pinned kernel.** The
  names are read from the kernel's `include/` directory at run time, plus
  `portmacro.h`. lwIP gets the same rule for the paths under
  `src/include`. This states the concern directly: one kernel, one tree. The
  compiler's own headers are exempt, since newlib has a `sys/queue.h` of its
  own that no kernel include could have resolved to.
- **A name the target owns resolves only in the target.** Every header read
  from under `Bdd/Targets/` is a name the target owns; a second header of the
  same name read from anywhere outside this checkout is a configuration
  header the build took from the wrong place.

The per-tree summary is printed as Markdown, and appended to the GitHub step
summary when `GITHUB_STEP_SUMMARY` is set. Beside it, when the target wires
CMSIS-FreeRTOS, the kernel version from the pinned tree and from the wrapper's
bundled copy: the rules above prove the build used one tree, not that the
wrapper was written for that kernel, so the pairing is shown rather than
gated.

Run:  python3 scripts/check_build_provenance.py --build build/cmsis-cross-lwip

Exit code:
    0  every file read is accounted for
    1  a rule above failed
    2  the build directory lacks a record this needs: the dependency files,
       the configure-time inputs, or the wired trees
"""

import argparse
import os
import pathlib
import re
import subprocess
import sys

# The subtrees of each upstream repository that a target may read, relative to
# the path the build was configured with. Anything else in the repository is
# an example, a test, a bundled copy of something pinned elsewhere, or a port
# for another architecture. A pattern with a `*` matches files directly under
# the root.
UPSTREAM_SUBTREES = {
    "SOLIDSYSLOG_FREERTOS_KERNEL_PATH": (
        "include",
        "portable/GCC/ARM_CM3",
        "portable/MemMang",
        "*.c",
    ),
    "SOLIDSYSLOG_FREERTOS_PLUS_TCP_PATH": ("source",),
    "SOLIDSYSLOG_FREERTOS_PLUS_FAT_PATH": ("include", "portable/common", "*.c"),
    # The lwIP targets take lwIP's own FreeRTOS port, not one of ours.
    "SOLIDSYSLOG_LWIP_PATH": ("src", "contrib/ports/freertos"),
    "SOLIDSYSLOG_CMSIS_PATH": ("CMSIS/RTOS2/Include", "CMSIS/Core/Include"),
    "SOLIDSYSLOG_CMSIS_FREERTOS_PATH": (
        "CMSIS/RTOS2/FreeRTOS/Source",
        "CMSIS/RTOS2/FreeRTOS/Include",
    ),
    # Mbed TLS's own build compiles its 3rdparty crypto (Everest, p256-m).
    "SOLIDSYSLOG_MBEDTLS_DIR": ("include", "library", "3rdparty"),
    "SOLIDSYSLOG_FATFS_PATH": ("source",),
    "SOLIDSYSLOG_LITTLEFS_PATH": ("*.c", "*.h"),
    "SOLIDSYSLOG_WOLFSSL_SOURCE_DIR": ("wolfssl", "src", "wolfcrypt/src"),
}

# Trees whose header names may resolve nowhere else. The value is the include
# directory the names are read from, relative to the configured path.
OWNED_NAME_TREES = {
    "SOLIDSYSLOG_FREERTOS_KERNEL_PATH": "include",
    "SOLIDSYSLOG_LWIP_PATH": "src/include",
}

# The port header is not under include/, and the bundled kernel has one too.
KERNEL_PORT_HEADER = "portmacro.h"

WIRED_TREES_FILE = "SolidSyslogUpstreamTrees.txt"
TARGET_DIRECTORY = "Bdd/Targets"
SOURCE_SUFFIXES = (".c", ".h")

CACHE_ENTRY = re.compile(r"^([A-Za-z0-9_]+):[A-Z]+=(.*)$")
IMPLICIT_INCLUDES = re.compile(r'set\(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "([^"]*)"\)')
MAKEFILE_DEPENDS = re.compile(r"set\(CMAKE_MAKEFILE_DEPENDS\s*(.*?)\)", re.DOTALL)
QUOTED = re.compile(r'"([^"]*)"')
NINJA_RERUN = re.compile(r"^build build\.ninja[^:]*: RERUN_CMAKE (.*?)$", re.MULTILINE)
KERNEL_VERSION = re.compile(r'#define\s+tskKERNEL_VERSION_NUMBER\s+"([^"]+)"')


class MissingRecord(Exception):
    """The build directory lacks something the check reads."""


def read_cache(build):
    entries = {}
    for line in (build / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        match = CACHE_ENTRY.match(line)
        if match:
            entries[match.group(1)] = match.group(2)
    return entries


def read_wired_trees(build):
    """The upstream trees the target declared, as cache variable names."""
    record = build / WIRED_TREES_FILE
    if not record.exists():
        raise MissingRecord(
            f"no {WIRED_TREES_FILE} under {build}: the configure did not reach a BDD target"
        )
    return [line for line in record.read_text(encoding="utf-8").split() if line]


def read_dependencies(build, cache):
    """Every file any object read, as the compiler wrote it. Ninja keeps the
    compiler's dependency files in its own log; Make leaves them on disk."""
    if (build / "build.ninja").exists():
        prerequisites = ninja_dependencies(build, cache.get("CMAKE_MAKE_PROGRAM", "ninja"))
    else:
        prerequisites = []
        for path in sorted(build.rglob("CMakeFiles/**/*.d")):
            if path.is_file():
                prerequisites.extend(parse_dependency_file(path.read_text(encoding="utf-8", errors="replace")))
    if not prerequisites:
        raise MissingRecord(f"no dependency records under {build}: was it built?")
    return prerequisites


def parse_dependency_file(text):
    """The prerequisites of one Make-style dependency file, source first."""
    text = text.replace("\\\n", " ")
    _, _, prerequisites = text.partition(":")
    return [
        item.replace("\\ ", " ")
        for item in re.split(r"(?<!\\)\s+", prerequisites.strip())
        if item
    ]


def ninja_dependencies(build, ninja):
    result = subprocess.run(
        [ninja, "-C", str(build), "-t", "deps"],
        capture_output=True,
        text=True,
        check=False,
    )
    return [line.strip() for line in result.stdout.splitlines() if line.startswith("    ")]


def configure_inputs(build):
    """The sources and headers CMake read at configure time, which is where a
    `configure_file(... COPYONLY)` copy in the build tree came from. Make
    records them in Makefile.cmake; Ninja as the inputs of the edge that
    re-runs CMake, in build.ninja or, for Ninja Multi-Config, the common
    file beside it."""
    makefile = build / "CMakeFiles" / "Makefile.cmake"
    if makefile.exists():
        match = MAKEFILE_DEPENDS.search(makefile.read_text(encoding="utf-8", errors="replace"))
        if match:
            return [item for item in QUOTED.findall(match.group(1)) if item.endswith(SOURCE_SUFFIXES)]
    for manifest in [build / "build.ninja", *sorted(build.glob("CMakeFiles/*.ninja"))]:
        if manifest.exists():
            text = manifest.read_text(encoding="utf-8", errors="replace").replace("$\n", "")
            match = NINJA_RERUN.search(text)
            if match:
                return [item for item in match.group(1).split() if item.endswith(SOURCE_SUFFIXES)]
    raise MissingRecord(f"no record of the configure-time inputs under {build}")


def resolve(prerequisite, build):
    """Make a prerequisite absolute. The compiler writes the paths as it was
    given them, and a relative one is relative to the build directory."""
    path = pathlib.Path(prerequisite)
    if not path.is_absolute():
        path = build / path
    return pathlib.Path(os.path.normpath(path))


def compiler_include_directories(build):
    """The directories the compiler searches on its own. CMake asked it at
    configure time and recorded the answer, so read that rather than ask
    again. Each is given both as recorded and with symlinks resolved, since
    the compiler writes a dependency by whichever spelling it searched."""
    directories = []
    for record in build.glob("CMakeFiles/*/CMakeCCompiler.cmake"):
        match = IMPLICIT_INCLUDES.search(record.read_text(encoding="utf-8"))
        for directory in match.group(1).split(";") if match else []:
            if not directory:
                continue
            path = pathlib.Path(directory)
            directories.append(path)
            if path.resolve() != path:
                directories.append(path.resolve())
    return directories


class AllowedSet:
    """The roots a file may come from, each named for the summary, and the
    pinned trees it may not, named for the failure."""

    def __init__(self):
        self.roots = []
        self.unwired = []

    def add(self, name, root, subtrees=None):
        self.roots.append((name, pathlib.Path(root), subtrees))

    def add_unwired(self, name, root):
        self.unwired.append((name, pathlib.Path(root)))

    def classify(self, path):
        """(tree name, problem). The problem is None for a file that is where
        it should be; the name is None for a file under no root at all."""
        for name, root, subtrees in self.roots:
            if path.is_relative_to(root):
                if subtrees is None or self.matches_subtree(path, root, subtrees):
                    return name, None
                return name, "inside the repository but outside its wired subtrees"
        for name, root in self.unwired:
            if path.is_relative_to(root):
                return name, f"inside {name}, a tree this target does not wire"
        return None, "outside every pinned tree"

    @staticmethod
    def matches_subtree(path, root, subtrees):
        relative = path.relative_to(root)
        for subtree in subtrees:
            if "*" in subtree:
                if len(relative.parts) == 1 and relative.match(subtree):
                    return True
            elif relative.is_relative_to(pathlib.Path(subtree)):
                return True
        return False


def tree_name(variable):
    return variable.removeprefix("SOLIDSYSLOG_")


def owned_names(cache, wired):
    """Relative include paths each wired pinned tree owns: {relative path: root}."""
    owned = {}
    for variable, include in OWNED_NAME_TREES.items():
        if variable not in wired or not cache.get(variable):
            continue
        root = pathlib.Path(cache[variable])
        include_root = root / include
        for header in include_root.rglob("*.h"):
            owned[header.relative_to(include_root).as_posix()] = root
        if variable == "SOLIDSYSLOG_FREERTOS_KERNEL_PATH":
            owned[KERNEL_PORT_HEADER] = root
    return owned


def ends_with(path, relative):
    parts = pathlib.PurePosixPath(relative).parts
    return path.parts[-len(parts) :] == parts


def kernel_version(task_header):
    try:
        text = task_header.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return "not found"
    match = KERNEL_VERSION.search(text)
    return match.group(1) if match else "not stated"


def check(files_read, allowed, wired, owned, exempt_from_owned, source_root):
    """Apply the rules. Returns (failures, counts by tree)."""
    failures = []
    counts = {}
    target_headers = {}
    for path in sorted(files_read):
        name, problem = allowed.classify(path)
        tree = (name if not problem else None) or "(unaccounted)"
        counts[tree] = counts.get(tree, 0) + 1
        if problem:
            failures.append(f"{path}: {problem}")
        if not any(path.is_relative_to(root) for root in exempt_from_owned):
            for relative, root in owned.items():
                if ends_with(path, relative) and not path.is_relative_to(root):
                    failures.append(f"{path}: {relative} is owned by {root}")
        if path.suffix == ".h" and path.is_relative_to(source_root / TARGET_DIRECTORY):
            target_headers.setdefault(path.name, path)
    for path in sorted(files_read):
        if path.name in target_headers and not path.is_relative_to(source_root):
            failures.append(
                f"{path}: {path.name} is owned by the target, at {target_headers[path.name]}"
            )
    for variable in wired:
        if tree_name(variable) not in counts:
            failures.append(f"{variable}: wired by the target, but nothing was read from it")
    return failures, counts


def summary(build, counts, files_read, cache, wired):
    lines = [
        f"### Provenance of `{build.name}`",
        "",
        f"{len(files_read)} distinct files read, by tree:",
        "",
        "| Tree | Files |",
        "|---|---|",
    ]
    for name in sorted(counts):
        lines.append(f"| {name} | {counts[name]} |")
    kernel = cache.get("SOLIDSYSLOG_FREERTOS_KERNEL_PATH")
    wrapper = cache.get("SOLIDSYSLOG_CMSIS_FREERTOS_PATH")
    if kernel and wrapper and "SOLIDSYSLOG_CMSIS_FREERTOS_PATH" in wired:
        pinned = kernel_version(pathlib.Path(kernel) / "include" / "task.h")
        bundled = kernel_version(pathlib.Path(wrapper) / "Source" / "include" / "task.h")
        lines.append("")
        lines.append(
            f"Kernel version pairing: the pinned kernel is {pinned}; "
            f"the kernel CMSIS-FreeRTOS bundles is {bundled}."
        )
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build",
        type=pathlib.Path,
        required=True,
        help="a configured and built build directory",
    )
    parser.add_argument(
        "--allow",
        action="append",
        default=[],
        metavar="DIR",
        help="a further directory to allow; the compiler's own include "
        "directories are read from CMake's record of them",
    )
    arguments = parser.parse_args(argv)
    build = arguments.build.resolve()

    cache = read_cache(build)
    source_root = pathlib.Path(cache["CMAKE_HOME_DIRECTORY"])

    try:
        wired = read_wired_trees(build)
        prerequisites = read_dependencies(build, cache)
        files_read = {resolve(item, build) for item in prerequisites + configure_inputs(build)}
    except MissingRecord as missing:
        print(missing)
        return 2

    compiler_directories = compiler_include_directories(build)
    allowed = AllowedSet()
    # The build tree usually sits inside the checkout: first match wins.
    allowed.add("build tree", build)
    allowed.add("this checkout", source_root)
    for directory in compiler_directories:
        allowed.add("compiler", directory)
    if cache.get("CMAKE_ROOT"):
        allowed.add("cmake", cache["CMAKE_ROOT"])
    for directory in arguments.allow:
        allowed.add(directory, pathlib.Path(directory).resolve())
    failures = []
    for variable in wired:
        if variable not in UPSTREAM_SUBTREES:
            failures.append(f"{variable}: wired by the target, but this script has no subtree rule for it")
        elif cache.get(variable):
            allowed.add(tree_name(variable), cache[variable], UPSTREAM_SUBTREES[variable])
    for variable in UPSTREAM_SUBTREES:
        if variable not in wired and cache.get(variable):
            allowed.add_unwired(tree_name(variable), cache[variable])

    more_failures, counts = check(
        files_read, allowed, wired, owned_names(cache, wired), compiler_directories, source_root
    )
    failures.extend(more_failures)

    text = summary(build, counts, files_read, cache, wired)
    print(text)
    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if step_summary:
        with open(step_summary, "a", encoding="utf-8") as handle:
            handle.write(text)

    for failure in failures:
        print(f"::error::{failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
