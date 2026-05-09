"""Target-driver abstraction for spawning the example-under-test.

The same Behave feature files run on multiple target platforms — Linux,
Windows, FreeRTOS-on-QEMU. The platforms differ only in how the
example-under-test is spawned: a native subprocess on Linux/Windows, or
qemu-system-arm with the cross-built ELF on FreeRTOS. Both backends
expose the same `SolidSyslog> ` prompt protocol over stdin/stdout
(printf-to-UART for FreeRTOS, printf-to-pipe for the native binaries),
so the existing wait_for_prompt / send_command helpers in
syslog_steps.py work unchanged once the spawn returns a Popen-like
object.

The active target is selected by the BDD_TARGET environment variable,
read in environment.before_all and stashed on context.target. Steps
call spawn_example_process(context, extra_args=...) instead of
subprocess.Popen directly.
"""

import os
import subprocess


# QEMU machine + UART config matches build-freertos-target's smoke run
# in .github/workflows/ci.yml: mps2-an385 / Cortex-M3 / 16 MiB / LAN9118
# NIC / -serial stdio for the polled CMSDK UART. -icount keeps virtual
# time decoupled from host load so timing-sensitive scenarios are
# deterministic under CI noise.
_QEMU_BASE_ARGS = [
    "qemu-system-arm",
    "-M", "mps2-an385",
    "-m", "16M",
    "-display", "none",
    "-serial", "stdio",
    "-icount", "shift=auto,sleep=off,align=off",
    "-netdev", "user,id=net0",
    "-net", "nic,netdev=net0,model=lan9118",
]


def spawn_example_process(context, extra_args=None, binary=None):
    """Spawn the example-under-test for the active target.

    Returns a subprocess.Popen with text-mode stdin/stdout/stderr pipes
    so the caller can use the existing prompt-protocol helpers
    (wait_for_prompt / send_command) without modification.

    extra_args are appended to the binary's command line on Linux and
    Windows (where the example-runner parses argv via getopt). FreeRTOS
    has no getopt port and consumes its config via interactive `set`
    commands instead; passing extra_args on FreeRTOS therefore raises —
    such scenarios should be tagged @freertoswip until a follow-up
    slice teaches the FreeRTOS driver to translate cmdline flags into
    the equivalent set commands.

    binary defaults to context.example_binary (the standard SingleTask
    binary or its FreeRTOS .elf equivalent); callers that drive a
    different example (e.g. the Linux Threaded binary) pass it
    explicitly. The FreeRTOS path only ever sees the SingleTask .elf
    today — threaded scenarios are tagged @freertoswip until a
    FreeRTOS threaded example exists.
    """
    target = getattr(context, "target", "linux")
    if binary is None:
        binary = context.example_binary

    if target == "freertos":
        if extra_args:
            raise NotImplementedError(
                "FreeRTOS target does not accept argv; tag the scenario "
                "@freertoswip until the driver translates flags into "
                "interactive `set` commands. Got: " + repr(extra_args)
            )
        cmd = list(_QEMU_BASE_ARGS) + ["-kernel", os.path.abspath(binary)]
    else:
        cmd = [os.path.abspath(binary)]
        if extra_args:
            cmd.extend(extra_args)

    return subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def stop_example_process(process, target, timeout=10):
    """Tear down the example process for the active target.

    Linux/Windows: the example exits cleanly on `quit`, so wait for the
    return code. FreeRTOS: `quit` only deletes the interactive task —
    the QEMU VM keeps idling (the FreeRTOS scheduler stays alive so a
    GDB attach works), so the only way to terminate is to kill QEMU
    directly. The BDD scenario has already verified the oracle
    received the frame, so the QEMU exit code carries no useful
    signal — return None for that path so callers don't assert on it.
    """
    if target == "freertos":
        process.kill()
        process.wait(timeout=timeout)
        return None

    process.stdin.write("quit\n")
    process.stdin.flush()
    process.wait(timeout=timeout)
    return process.returncode
