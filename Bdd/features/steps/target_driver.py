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
subprocess.Popen directly. After wait_for_prompt, callers also invoke
apply_extra_args so any `--flag value` pairs are delivered to the
target — appended to argv on Linux/Windows (a no-op there because
spawn already did it), or translated into `set NAME VALUE` lines over
the UART on FreeRTOS.
"""

import os
import subprocess


# Mapping from cmdline flag to FreeRTOS interactive `set` name. Only the
# flags features currently pass to run_example are mapped — adding a new
# scenario that needs a new flag should fail loudly via the
# `_FREERTOS_SET_TRANSLATION` lookup so the gap is visible, rather than
# silently no-op'ing or printing a confusing "set: invalid" on the UART.
# `--message` -> `msg` is intentional: the FreeRTOS example global is
# g_msg (Example/FreeRtos/SingleTask/main.c) and renaming the set name
# would churn the example for cosmetic gain.
_FREERTOS_SET_TRANSLATION = {
    "--facility": "facility",
    "--severity": "severity",
    "--msgid": "msgid",
    "--message": "msg",
}


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
    commands instead — they are delivered via apply_extra_args after
    the prompt is up.

    binary defaults to context.example_binary (the standard SingleTask
    binary or its FreeRTOS .elf equivalent); callers that drive a
    different example (e.g. the Linux Threaded binary) pass it
    explicitly. On FreeRTOS, the SingleTask .elf is itself buffered
    (CircularBuffer + FreeRtosMutex + Service task, S08.04) so it
    serves both the @buffered scenarios and the plain single-message
    path — there is no separate FreeRTOS threaded binary.
    """
    target = getattr(context, "target", "linux")
    if binary is None:
        binary = context.example_binary

    if target == "freertos":
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


def apply_extra_args(context, process, extra_args):
    """Deliver `--flag value` pairs to the example after the prompt is up.

    Linux and Windows are no-ops here — spawn already placed the flags
    in argv. FreeRTOS translates each pair via _FREERTOS_SET_TRANSLATION
    and writes one newline-terminated `set NAME VALUE` line to the UART
    per pair.

    Raises ValueError if extra_args is odd-length or contains a flag
    not in the translation table; the BDD scenario surfaces the gap
    immediately rather than silently no-op'ing or hitting a confusing
    UART-side `set: invalid` reply.
    """
    if not extra_args:
        return
    target = getattr(context, "target", "linux")
    if target != "freertos":
        return
    if len(extra_args) % 2 != 0:
        raise ValueError(
            "FreeRTOS extra_args must be flag/value pairs; got odd length: "
            + repr(extra_args)
        )
    for flag, value in zip(extra_args[0::2], extra_args[1::2]):
        try:
            name = _FREERTOS_SET_TRANSLATION[flag]
        except KeyError as exc:
            raise ValueError(
                f"Unknown cmdline flag for FreeRTOS target: {flag!r}. "
                "Add it to _FREERTOS_SET_TRANSLATION in target_driver.py "
                "if a corresponding `set` name exists in the FreeRTOS "
                "example's OnSet handler."
            ) from exc
        process.stdin.write(f"set {name} {value}\n")
    process.stdin.flush()


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
