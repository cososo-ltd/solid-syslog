import glob
import logging
import os
import platform
import shutil
import socket
import subprocess
import time

logger = logging.getLogger("behave.environment")

SYSLOG_NG_CONF = "Bdd/syslog-ng/syslog-ng.conf"
SYSLOG_NG_FULL_CONF = "Bdd/syslog-ng/syslog-ng-full.conf"
SYSLOG_NG_CTL = "/var/lib/syslog-ng/syslog-ng.ctl"

# Store-and-forward backing files. POSIX uses /tmp (Linux Threaded example
# hardcodes this); Windows uses Bdd/output/ relative to the working dir
# (Windows Example hardcodes this). Both runners run behave from the project
# root so the relative path resolves cleanly. Keep the Python side aligned
# with whichever path the binary is using on each platform.
if platform.system() == "Windows":
    _STORE_DIR = os.path.join("Bdd", "output")
    STORE_FILE_PATH = os.path.join(_STORE_DIR, "solidsyslog_store.dat")
    STORE_PATH_PREFIX = os.path.join(_STORE_DIR, "STORE")
    THRESHOLD_MARKER_PATH = os.path.join(_STORE_DIR, "solidsyslog_threshold_marker.log")
else:
    STORE_FILE_PATH = "/tmp/solidsyslog_store.dat"
    STORE_PATH_PREFIX = "/tmp/STORE"
    THRESHOLD_MARKER_PATH = "/tmp/solidsyslog_threshold_marker.log"
RECEIVED_UDP_LOG = "Bdd/output/received_udp.log"
RECEIVED_TCP_LOG = "Bdd/output/received_tcp.log"
RECEIVED_TLS_LOG = "Bdd/output/received_tls.log"
RECEIVED_MTLS_LOG = "Bdd/output/received_mtls.log"


def wait_for_tcp_port_open(host="syslog-ng", port=5514, timeout=5):
    """Poll until the TCP port accepts connections."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(0.5)
                s.connect((host, port))
            return
        except (ConnectionRefusedError, OSError):
            time.sleep(0.1)
    raise AssertionError(f"TCP port {port} not open after {timeout}s")


def otel_kill_oracle():
    """Stop any running otelcol-contrib (Windows). Idempotent: returns
    cleanly if no process is running.
    """
    subprocess.run(
        ["taskkill", "/F", "/IM", "otelcol-contrib.exe"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )


OTELCOL_BIN    = os.path.join("Bdd", "otel", "bin", "otelcol-contrib.exe")
OTELCOL_CONFIG = os.path.join("Bdd", "otel", "config.yaml")
OTELCOL_OUT    = os.path.join("Bdd", "output", "otelcol.out")
OTELCOL_ERR    = os.path.join("Bdd", "output", "otelcol.err")


def otel_start_oracle():
    """Start a fresh otelcol-contrib (Windows) with the BDD config and wait
    for the TCP/UDP/TLS/mTLS listeners to bind. Mirrors what the CI workflow
    does on first start (otelcol-contrib.exe + Bdd/otel/config.yaml,
    stdout/err appended to Bdd/output/otelcol.{out,err}). Closes the
    parent's stdout/err handles after Popen so they don't leak — the child
    keeps its own duplicated copies. Uses os.path.join so the executable
    path has Windows-native backslashes — _winapi.CreateProcess does not
    resolve forward slashes the way bash does.

    Wait on all three TCP-bound ports (5514 syslog/TCP, 6514 TLS, 6515 mTLS)
    rather than just 5514. Otelcol binds them in series during startup, so
    a kill+restart in a previous scenario followed immediately by an mTLS
    scenario could race the TLS handshake against an unbound listener.
    """
    os.makedirs(os.path.dirname(OTELCOL_OUT), exist_ok=True)
    with open(OTELCOL_OUT, "ab") as out, open(OTELCOL_ERR, "ab") as err:
        subprocess.Popen(
            [OTELCOL_BIN, "--config=" + OTELCOL_CONFIG],
            stdout=out,
            stderr=err,
        )
    for port in (5514, 6514, 6515):
        wait_for_tcp_port_open(host="127.0.0.1", port=port, timeout=15)


def before_all(context):
    # Configurable via env so the same step code drives Linux (syslog-ng) and
    # Windows (OTel Collector) runners with different binary paths and oracle
    # output formats.
    context.example_binary = os.environ.get(
        "EXAMPLE_BINARY", "build/debug/Example/SolidSyslogExample"
    )
    context.received_log = os.environ.get(
        "RECEIVED_LOG", "Bdd/output/received.log"
    )
    context.oracle_format = os.environ.get("ORACLE_FORMAT", "syslog-ng")

    # On the Windows runner the OTel oracle binds 127.0.0.1; on Linux the
    # example reaches syslog-ng via the docker compose service name. The
    # example reads SOLIDSYSLOG_BDD_TLS_HOST / _MTLS_HOST at startup and
    # falls back to "syslog-ng" when unset. Use a truthiness check rather
    # than setdefault so a pre-existing empty-string env var also gets
    # the loopback default — empty would otherwise pass through and break
    # name resolution.
    if context.oracle_format == "otel-jsonl":
        for key in ("SOLIDSYSLOG_BDD_TLS_HOST", "SOLIDSYSLOG_BDD_MTLS_HOST"):
            if not os.environ.get(key):
                os.environ[key] = "127.0.0.1"


def after_scenario(context, scenario):
    # Clean up any long-lived interactive process
    if hasattr(context, "interactive_process"):
        process = context.interactive_process
        if process.poll() is None:
            try:
                process.stdin.write("quit\n")
                process.stdin.flush()
            except (BrokenPipeError, OSError):
                pass
            try:
                process.wait(timeout=5)
            except Exception:
                process.kill()
                process.wait()
        del context.interactive_process

    # Restore syslog-ng config if it was changed during the scenario.
    # copyfile (data only, no chmod) — see syslog_ng_swap_config rationale.
    if hasattr(context, "syslog_ng_config_changed") and context.syslog_ng_config_changed:
        try:
            shutil.copyfile(SYSLOG_NG_FULL_CONF, SYSLOG_NG_CONF)
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.connect(SYSLOG_NG_CTL)
                sock.sendall(b"RELOAD\n")
                sock.recv(1024)
            wait_for_tcp_port_open()
        except Exception as exc:
            logger.warning("Failed to restore syslog-ng config in teardown: %s", exc)
        context.syslog_ng_config_changed = False

    # Restart the OTel oracle if a "stops accepting TCP" step killed it during
    # this scenario. taskkill takes down all four listeners (UDP/TCP/TLS/mTLS)
    # and `Given the syslog oracle is running` only records counts — it does
    # not start anything — so without this every subsequent scenario fails.
    if (getattr(context, "oracle_format", None) == "otel-jsonl"
            and getattr(context, "otel_oracle_paused", False)):
        try:
            otel_start_oracle()
            context.otel_oracle_paused = False
        except Exception as exc:
            logger.warning("Failed to restart otelcol-contrib in teardown: %s", exc)

    # Clean up store files to prevent cross-scenario contamination
    try:
        os.remove(STORE_FILE_PATH)
    except FileNotFoundError:
        pass
    for path in glob.glob(STORE_PATH_PREFIX + "*.log"):
        os.remove(path)
    try:
        os.remove(THRESHOLD_MARKER_PATH)
    except FileNotFoundError:
        pass
