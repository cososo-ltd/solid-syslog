import glob
import logging
import os
import shutil
import socket
import time

logger = logging.getLogger("behave.environment")

SYSLOG_NG_CONF = "Bdd/syslog-ng/syslog-ng.conf"
SYSLOG_NG_FULL_CONF = "Bdd/syslog-ng/syslog-ng-full.conf"
SYSLOG_NG_CTL = "/var/lib/syslog-ng/syslog-ng.ctl"
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

    # Restore syslog-ng config if it was changed during the scenario
    if hasattr(context, "syslog_ng_config_changed") and context.syslog_ng_config_changed:
        shutil.copy(SYSLOG_NG_FULL_CONF, SYSLOG_NG_CONF)
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.connect(SYSLOG_NG_CTL)
                sock.sendall(b"RELOAD\n")
                sock.recv(1024)
            wait_for_tcp_port_open()
        except Exception as exc:
            logger.warning("Failed to restore syslog-ng config in teardown: %s", exc)
        context.syslog_ng_config_changed = False

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
