import glob
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import time
from datetime import datetime, timezone

from behave import given, when, then
from environment import (
    RECEIVED_MTLS_LOG,
    RECEIVED_TCP_LOG,
    RECEIVED_TLS_LOG,
    RECEIVED_UDP_LOG,
    STORE_FILE_PATH,
    STORE_PATH_PREFIX,
    THRESHOLD_MARKER_PATH,
)

PER_TRANSPORT_LOG = {
    "udp": RECEIVED_UDP_LOG,
    "tcp": RECEIVED_TCP_LOG,
    "tls": RECEIVED_TLS_LOG,
    "mtls": RECEIVED_MTLS_LOG,
}

# POSIX-only paths used by the @buffered/threaded scenarios. The cross-platform
# scenarios use context.example_binary / context.received_log instead, set in
# environment.before_all from EXAMPLE_BINARY / RECEIVED_LOG / ORACLE_FORMAT.
THREADED_BINARY = "build/debug/Example/SolidSyslogThreadedExample"
SYSLOG_NG_CTL = "/var/lib/syslog-ng/syslog-ng.ctl"
SYSLOG_NG_CONF = "Bdd/syslog-ng/syslog-ng.conf"
SYSLOG_NG_FULL_CONF = "Bdd/syslog-ng/syslog-ng-full.conf"
SYSLOG_NG_UDP_ONLY_CONF = "Bdd/syslog-ng/syslog-ng-udp-only.conf"

# Mirrors SOLIDSYSLOG_MAX_MESSAGE_SIZE from Core/Interface/SolidSyslog.h. Bump
# the two together. The store_capacity scenarios depend on it because production
# clamps max-file-size up to MAX + RECORD_OVERHEAD + integritySize at runtime,
# so the file size used by the file store is MAX-coupled even when the feature
# file specifies a smaller value.
SOLIDSYSLOG_MAX_MESSAGE_SIZE = 2048


def clean_store_files():
    """Remove all rotating store files matching the path prefix."""
    for path in glob.glob(STORE_PATH_PREFIX + "*.log"):
        os.remove(path)


def line_count(path):
    """Return the number of lines in a file, or 0 if it doesn't exist."""
    if not os.path.exists(path):
        return 0
    with open(path, encoding="utf-8") as f:
        return sum(1 for _ in f)


def read_new_lines(path, skip):
    """Return all lines after the first 'skip' lines."""
    with open(path, encoding="utf-8") as f:
        lines = f.readlines()
    return [line.strip() for line in lines[skip:] if line.strip()]


def _split_otel_line_into_records(line):
    """OTel's file exporter emits one JSON object per write but each object
    can carry multiple log records when messages arrive close together.
    Yield one single-record envelope (still parse_otel_jsonl_line-compatible)
    per logical record."""
    parent = json.loads(line)
    for resource_log in parent.get("resourceLogs", []):
        for scope_log in resource_log.get("scopeLogs", []):
            for log_record in scope_log.get("logRecords", []):
                yield json.dumps({
                    "resourceLogs": [{
                        "resource": resource_log.get("resource", {}),
                        "scopeLogs": [{
                            "scope": scope_log.get("scope", {}),
                            "logRecords": [log_record],
                        }],
                    }]
                })


def oracle_record_count(path, oracle_format):
    """Total number of logical oracle records in the file. For syslog-ng each
    line is one record; for OTel JSONL each line may carry several."""
    if not os.path.exists(path):
        return 0
    if oracle_format != "otel-jsonl":
        return line_count(path)
    count = 0
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            count += sum(1 for _ in _split_otel_line_into_records(line))
    return count


def read_new_oracle_records(path, oracle_format, skip):
    """Return all logical oracle records after the first 'skip', each as a
    string ready for parse_oracle_line."""
    if not os.path.exists(path):
        return []
    records = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if oracle_format != "otel-jsonl":
                records.append(line)
            else:
                records.extend(_split_otel_line_into_records(line))
    return records[skip:]


def read_last_line(path):
    """Return the last non-empty line from a file."""
    with open(path, encoding="utf-8") as f:
        lines = f.readlines()
    for line in reversed(lines):
        if line.strip():
            return line.strip()
    return ""


UTF8_BOM_CHAR = "\ufeff"


def _strip_msg_bom(msg):
    """The library emits the RFC 5424 §6.4 UTF-8 BOM at the start of every MSG
    (S12.13 #219). It's a transport-layer prefix, not part of the body the
    test author cares about, so strip it on the receive side so that
    MSG-content assertions can compare the body bytes directly."""
    if msg.startswith(UTF8_BOM_CHAR):
        return msg[len(UTF8_BOM_CHAR):]
    return msg


def parse_syslog_ng_line(line):
    """Parse a syslog-ng key=value template line into a dict."""
    fields = {}
    for match in re.finditer(r"(\w+)=(\S+)", line):
        fields[match.group(1)] = match.group(2)

    # STRUCTURED_DATA may contain spaces and special chars — capture between
    # "STRUCTURED_DATA=" and " MSG="
    sd_match = re.search(r"STRUCTURED_DATA=(.*?) MSG=", line)
    if sd_match:
        fields["STRUCTURED_DATA"] = sd_match.group(1)

    # MSG may contain spaces — capture everything after "MSG="
    msg_match = re.search(r"MSG=(.*)", line)
    if msg_match:
        fields["MSG"] = _strip_msg_bom(msg_match.group(1))

    return fields


def _otel_attribute(attrs, key):
    """Pull a single attribute value (str/int) out of the OTel attribute list."""
    for attr in attrs:
        if attr.get("key") == key:
            value = attr.get("value", {})
            if "stringValue" in value:
                return value["stringValue"]
            if "intValue" in value:
                return str(value["intValue"])
            return value
    return None


def _render_otel_structured_data(sd_value):
    """Render OTel's nested kvlist structured-data attribute back into the
    syslog-ng `[sd-id k1="v1" k2="v2"]...` text form, so the existing
    regex-based Then steps don't need per-oracle branching.

    OTel shape (from the syslog receiver in rfc5424 mode):
      {"kvlistValue":{"values":[
        {"key":"meta", "value":{"kvlistValue":{"values":[
          {"key":"sequenceId", "value":{"stringValue":"1"}}]}}},
        ...
      ]}}
    """
    elements = sd_value.get("kvlistValue", {}).get("values", [])
    out = []
    for sd_element in elements:
        sd_id = sd_element.get("key", "")
        params = sd_element.get("value", {}).get("kvlistValue", {}).get("values", [])
        rendered_params = []
        for param in params:
            param_name = param.get("key", "")
            param_val = param.get("value", {})
            param_str = param_val.get("stringValue") or str(param_val.get("intValue", ""))
            rendered_params.append(f'{param_name}="{param_str}"')
        if rendered_params:
            out.append(f"[{sd_id} {' '.join(rendered_params)}]")
        else:
            out.append(f"[{sd_id}]")
    return "".join(out)


def parse_otel_jsonl_line(line):
    """Parse one JSON line from the OTel Collector file exporter into the
    same flat dict shape as parse_syslog_ng_line.

    Maps OTel's parsed RFC 5424 attributes (independent oracle, same role
    syslog-ng plays on Linux) into the flat field dict.
    """
    record = json.loads(line)
    log = record["resourceLogs"][0]["scopeLogs"][0]["logRecords"][0]
    attrs = log.get("attributes", [])

    fields = {}
    priority = _otel_attribute(attrs, "priority")
    if priority is not None:
        fields["PRIORITY"] = priority
    hostname = _otel_attribute(attrs, "hostname")
    if hostname is not None:
        fields["HOSTNAME"] = hostname
    appname = _otel_attribute(attrs, "appname")
    if appname is not None:
        fields["APP_NAME"] = appname
    proc_id = _otel_attribute(attrs, "proc_id")
    if proc_id is not None:
        fields["PROCID"] = proc_id
    msg_id = _otel_attribute(attrs, "msg_id")
    if msg_id is not None:
        fields["MSGID"] = msg_id
    msg = _otel_attribute(attrs, "message")
    if msg is not None:
        fields["MSG"] = _strip_msg_bom(msg)

    sd = _otel_attribute(attrs, "structured_data")
    if isinstance(sd, dict):
        fields["STRUCTURED_DATA"] = _render_otel_structured_data(sd)

    # timeUnixNano → ISO 8601 string compatible with datetime.fromisoformat
    time_ns = log.get("timeUnixNano")
    if time_ns is not None:
        seconds = int(time_ns) / 1e9
        fields["TIMESTAMP"] = datetime.fromtimestamp(seconds, tz=timezone.utc).isoformat()

    return fields


def parse_oracle_line(line, oracle_format):
    """Dispatch to the right parser based on the active oracle."""
    if oracle_format == "otel-jsonl":
        return parse_otel_jsonl_line(line)
    return parse_syslog_ng_line(line)


def wait_for_prompt(process, timeout=30):
    """Read stdout until we see 'SolidSyslog> ', confirming the command completed."""
    import select

    fd = process.stdout.fileno()
    output = b""
    deadline = time.monotonic() + timeout
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError(
                f"Timed out waiting for prompt after {timeout}s. "
                f"Output so far: {output.decode(errors='replace')}"
            )
        ready, _, _ = select.select([fd], [], [], min(remaining, 0.5))
        if not ready:
            continue
        data = os.read(fd, 1)
        if not data:
            break
        output += data
        if output.endswith(b"SolidSyslog> "):
            return output.decode()
    return output.decode()


def send_command(process, command):
    """Send a command to the interactive process and wait for the prompt."""
    process.stdin.write(command + "\n")
    process.stdin.flush()
    return wait_for_prompt(process)


def wait_for_messages(context, expected_messages):
    """Wait for the oracle to flush the expected number of new records.
    Counts logical records (not raw file lines) so OTel's batch-per-line
    write behaviour doesn't undercount."""
    received_log = context.received_log
    oracle_format = context.oracle_format
    expected_total = context.lines_before + expected_messages
    deadline = time.monotonic() + 5
    while oracle_record_count(received_log, oracle_format) < expected_total:
        if time.monotonic() > deadline:
            actual = oracle_record_count(received_log, oracle_format) - context.lines_before
            raise AssertionError(
                f"oracle received {actual} of {expected_messages} "
                f"messages within 5 seconds"
            )
        time.sleep(0.1)

    context.all_lines = read_new_oracle_records(received_log, oracle_format, context.lines_before)
    context.fields = parse_oracle_line(context.all_lines[-1], oracle_format)
    context.message_count = len(context.all_lines)


def run_example(context, extra_args=None, binary=None, expected_messages=1):
    """Run the single-task example as a one-shot: write all stdin upfront,
    wait for clean exit, then assert the oracle saw the messages.

    Portable across Linux and Windows — no select.select on a pipe fd (which
    Windows doesn't support). Single-task example only: every Log call is
    synchronous (NullBuffer + Datagram), so "quit" cannot arrive before the
    UDP packets have left the socket.
    """
    binary = binary or context.example_binary
    assert os.path.exists(binary), (
        f"Example binary not found at {binary} — build with cmake first"
    )

    # Windows CreateProcess needs an absolute path (or one resolvable via PATH);
    # forward-slash relative paths from a bash-launched behave fail otherwise.
    cmd = [os.path.abspath(binary)]
    if extra_args:
        cmd.extend(extra_args)

    process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    context.example_pid = process.pid

    try:
        process.communicate(input=f"send {expected_messages}\nquit\n", timeout=15)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
        raise

    assert process.returncode == 0, (
        f"Example binary failed with exit code {process.returncode}"
    )

    wait_for_messages(context, expected_messages)


def run_threaded_example(context, extra_args=None, expected_messages=1):
    """Run the threaded example using the prompt-based protocol so that the
    service thread has time to drain the buffer between "send N" and "quit".

    POSIX-only — uses select.select on a pipe fd, which Windows does not
    support. All threaded scenarios are tagged @buffered and excluded from
    the Windows runner.
    """
    binary = THREADED_BINARY
    assert os.path.exists(binary), (
        f"Threaded binary not found at {binary} — build with cmake first"
    )

    cmd = [os.path.join(".", binary)]
    if extra_args:
        cmd.extend(extra_args)

    process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    context.example_pid = process.pid

    try:
        wait_for_prompt(process)
        send_command(process, f"send {expected_messages}")
        wait_for_messages(context, expected_messages)

        process.stdin.write("quit\n")
        process.stdin.flush()
        process.wait(timeout=10)
        assert process.returncode == 0, (
            f"Threaded example failed with exit code {process.returncode}"
        )
    finally:
        # Don't let an intermediate exception leak the helper into later
        # scenarios — kill if it's still running after the assertions above.
        if process.poll() is None:
            process.kill()
            process.wait(timeout=5)


def syslog_ng_reload():
    """Send RELOAD to syslog-ng via its Unix control socket."""
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(SYSLOG_NG_CTL)
        sock.sendall(b"RELOAD\n")
        response = sock.recv(1024)
    assert b"OK" in response, f"syslog-ng reload failed: {response}"
    # Allow time for syslog-ng to complete the reload
    time.sleep(0.5)


def syslog_ng_swap_config(config_path):
    """Replace the active syslog-ng config and reload."""
    shutil.copy(config_path, SYSLOG_NG_CONF)
    syslog_ng_reload()


@given("syslog-ng is running")
def step_syslog_ng_is_running(context):
    # Only assert the syslog-ng control socket when syslog-ng is actually the
    # active oracle. Other runners (e.g. the OTel Collector on Windows) reuse
    # this step text but expose no such socket.
    if context.oracle_format == "syslog-ng":
        assert os.path.exists(SYSLOG_NG_CTL), (
            f"syslog-ng control socket not found at {SYSLOG_NG_CTL}"
        )

    # Record current logical-record count so we can detect the new message.
    # Field name kept (`lines_before`) for legacy callers, but for OTel one
    # JSONL file line may carry several records.
    context.lines_before = oracle_record_count(context.received_log, context.oracle_format)

    # Per-transport baselines for the SwitchingSender scenarios. Only the
    # syslog-ng oracle emits these; other oracles leave the dict empty.
    context.lines_before_per_transport = {}
    if context.oracle_format == "syslog-ng":
        for transport, path in PER_TRANSPORT_LOG.items():
            context.lines_before_per_transport[transport] = line_count(path)


def build_threaded_command(context, transport, no_sd=False):
    """Build the command line for the threaded example with all options."""
    binary = THREADED_BINARY
    assert os.path.exists(binary), (
        f"Threaded binary not found at {binary} — build with cmake first"
    )

    cmd = [os.path.join(".", binary), "--transport", transport]
    if getattr(context, "store_type", None):
        cmd.extend(["--store", context.store_type])
    if getattr(context, "store_max_files", None):
        cmd.extend(["--max-files", str(context.store_max_files)])
    if getattr(context, "store_max_file_size", None):
        cmd.extend(["--max-file-size", str(context.store_max_file_size)])
    if getattr(context, "store_discard_policy", None):
        cmd.extend(["--discard-policy", context.store_discard_policy])
    if getattr(context, "capacity_threshold", None):
        cmd.extend(["--capacity-threshold", str(context.capacity_threshold)])
    if getattr(context, "message_body", None):
        cmd.extend(["--message", context.message_body])
    if no_sd:
        cmd.append("--no-sd")
    if getattr(context, "halt_exit", False):
        cmd.append("--halt-exit")
    return cmd


def start_threaded_example(context, cmd):
    """Start the threaded example process and wait for the initial prompt."""
    context.interactive_process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    context.example_pid = context.interactive_process.pid
    wait_for_prompt(context.interactive_process)


@given("the threaded example is running with transport {transport:w}")
def step_threaded_running_with_transport(context, transport):
    cmd = build_threaded_command(context, transport)
    start_threaded_example(context, cmd)


@given("the threaded example is running with transport {transport:w} and no structured data")
def step_threaded_running_with_transport_no_sd(context, transport):
    cmd = build_threaded_command(context, transport, no_sd=True)
    start_threaded_example(context, cmd)


@given("the file store is enabled")
def step_file_store_enabled(context):
    context.store_type = "file"
    if os.path.exists(STORE_FILE_PATH):
        os.remove(STORE_FILE_PATH)


@given("the file store is enabled with max-files {max_files:d} and max-file-size {max_file_size:d} and discard-policy {policy}")
def step_file_store_enabled_with_config(context, max_files, max_file_size, policy):
    context.store_type = "file"
    context.store_max_files = max_files
    context.store_max_file_size = max_file_size
    context.store_discard_policy = policy
    # Size each MSG so ~4 records pack per (clamped) store file. The store
    # capacity scenarios were designed around this packing — multi-record
    # files give OLDEST and NEWEST symmetric retention (both keep 7 of 10
    # sent), which the seqId assertions depend on. Production clamps file
    # size up to MAX + 7 (MIN_MAX_FILE_SIZE), so per-record budget is
    # ~MAX/4. With ~95-byte RFC 5424 header + 7-byte record overhead, a
    # body of MAX/5 - 50 lands a comfortable mid-band: 4 records fit, 5
    # don't. Update if SOLIDSYSLOG_MAX_MESSAGE_SIZE moves.
    context.message_body = "X" * (SOLIDSYSLOG_MAX_MESSAGE_SIZE // 5 - 50)
    clean_store_files()


@given("the halt callback exits the process")
def step_halt_callback_exits(context):
    context.halt_exit = True


@given("the capacity threshold callback is enabled at {threshold:d} bytes")
def step_capacity_threshold_enabled(context, threshold):
    context.capacity_threshold = threshold
    try:
        os.remove(THRESHOLD_MARKER_PATH)
    except FileNotFoundError:
        pass


@then("the capacity threshold callback was invoked")
def step_threshold_callback_invoked(context):
    assert os.path.exists(THRESHOLD_MARKER_PATH), (
        f"Expected threshold marker at {THRESHOLD_MARKER_PATH}, found none"
    )
    assert os.path.getsize(THRESHOLD_MARKER_PATH) > 0, (
        f"Threshold marker {THRESHOLD_MARKER_PATH} exists but is empty"
    )


@then("the capacity threshold callback was not invoked")
def step_threshold_callback_not_invoked(context):
    assert not os.path.exists(THRESHOLD_MARKER_PATH) or os.path.getsize(THRESHOLD_MARKER_PATH) == 0, (
        f"Expected no threshold marker but found {THRESHOLD_MARKER_PATH} non-empty"
    )


@when("the client sends a message")
def step_client_sends_message(context):
    send_command(context.interactive_process, "send")
    # Allow time for the service thread to drain the buffer and send
    time.sleep(0.2)


def wait_for_tcp_port_closed(host="syslog-ng", port=5514, timeout=5):
    """Poll until the TCP port refuses connections."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(0.5)
                s.connect((host, port))
            time.sleep(0.1)
        except (ConnectionRefusedError, OSError):
            return
    raise AssertionError(f"TCP port {port} still open after {timeout}s")


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


def wait_for_connection_teardown(probe_socket, timeout=5):
    """Wait until an established TCP connection is broken by the server.

    Sends data on the probe socket until a broken pipe or reset indicates
    that syslog-ng has closed the connection after a config reload.
    """
    msg = b"<134>1 2026-01-01T00:00:00Z probe probe - - - probe"
    frame = f"{len(msg)} ".encode() + msg
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            probe_socket.sendall(frame)
            time.sleep(0.1)
        except (BrokenPipeError, ConnectionResetError, OSError):
            probe_socket.close()
            return
    probe_socket.close()
    raise AssertionError(f"Probe connection still alive after {timeout}s")


@when("the syslog server stops accepting TCP connections")
def step_syslog_server_stops_tcp(context):
    # Open a probe connection before the reload so we can detect teardown
    probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    probe.settimeout(1)
    probe.connect(("syslog-ng", 5514))

    syslog_ng_swap_config(SYSLOG_NG_UDP_ONLY_CONF)
    wait_for_tcp_port_closed()
    wait_for_connection_teardown(probe)
    # Allow time for the sender's existing connection to receive RST
    time.sleep(0.5)
    context.syslog_ng_config_changed = True


@when("the syslog server resumes accepting TCP connections")
def step_syslog_server_resumes_tcp(context):
    syslog_ng_swap_config(SYSLOG_NG_FULL_CONF)
    wait_for_tcp_port_open()


@when("the example program sends a syslog message")
def step_example_sends_message(context):
    run_example(context)


@when("the example program sends a syslog message with transport {transport}")
def step_example_sends_with_transport(context, transport):
    run_example(context, ["--transport", transport])


@when("the threaded example sends a syslog message")
def step_threaded_sends_message(context):
    run_threaded_example(context)


@when("the threaded example sends a syslog message with transport {transport}")
def step_threaded_sends_with_transport(context, transport):
    run_threaded_example(context, ["--transport", transport])


@when("the threaded example sends {count:d} syslog messages")
def step_threaded_sends_multiple(context, count):
    run_threaded_example(context, expected_messages=count)


@when("the example program sends a message with facility {facility:d} and severity {severity:d}")
def step_example_sends_with_facility_severity(context, facility, severity):
    run_example(context, ["--facility", str(facility), "--severity", str(severity)])


@when('the example program sends a message with message ID "{msgid}"')
def step_example_sends_with_msgid(context, msgid):
    run_example(context, ["--msgid", msgid])


@when('the example program sends a message with body "{body}"')
def step_example_sends_with_body(context, body):
    run_example(context, ["--message", body])


@when('the example program sends a complete message with message ID "{msgid}" and body "{body}"')
def step_example_sends_with_msgid_and_body(context, msgid, body):
    run_example(context, ["--msgid", msgid, "--message", body])


@when("the example program sends {count:d} syslog messages")
def step_example_sends_multiple(context, count):
    run_example(context, expected_messages=count)


@when("the example program sends a UTF-8 message that fits the path MTU")
def step_example_sends_utf8_within_mtu(context):
    # Comfortably under the typical 1472-byte path payload, with multi-byte
    # UTF-8 mixed in. Tests the no-trim path: the message goes out whole.
    msg = "Hello, " + ("é" * 100) + " - mixed " + ("€" * 50) + " - end"
    context.sent_msg = msg
    run_example(context, ["--message", msg])


@when("the example program sends an oversize UTF-8 message")
def step_example_sends_oversize_utf8(context):
    # Build a message that overflows the path MTU. The ASCII prefix keeps
    # the prefix-equality assertion robust; the long run of '€' (3 bytes
    # each) ensures the trim point almost certainly lands mid-codepoint,
    # exercising the walk-back to a clean codepoint boundary.
    # ~1600 MSG bytes + ~80 RFC 5424 header = ~1680 wire bytes,
    # well above the docker-bridge path MTU's 1472-byte payload limit.
    msg = ("X" * 100) + ("€" * 500)
    context.sent_msg = msg
    run_example(context, ["--message", msg])


@then("the received message is byte-identical to the sent message")
def step_check_msg_byte_identical(context):
    received = context.fields.get("MSG", "")
    assert received == context.sent_msg, (
        f"Expected {len(context.sent_msg.encode('utf-8'))} bytes byte-identical, "
        f"got {len(received.encode('utf-8'))} bytes that differ"
    )


@then("the received message is shorter than the sent message")
def step_check_msg_shorter(context):
    received = context.fields.get("MSG", "")
    sent_bytes = len(context.sent_msg.encode("utf-8"))
    received_bytes = len(received.encode("utf-8"))
    assert received_bytes < sent_bytes, (
        f"Expected trim — sent {sent_bytes} bytes, received {received_bytes}"
    )


@then("the received message is a clean prefix of the sent message")
def step_check_msg_clean_prefix(context):
    received = context.fields.get("MSG", "")
    assert context.sent_msg.startswith(received), (
        "Received is not a clean prefix of sent — trim left orphan bytes "
        "or modified content. "
        f"Sent starts: {context.sent_msg[:60]!r}; "
        f"received starts: {received[:60]!r}; "
        f"received ends: {received[-40:]!r}"
    )


@then('syslog-ng receives a message with priority "{priority}"')
def step_check_priority(context, priority):
    assert context.fields["PRIORITY"] == priority, (
        f"Expected priority {priority}, got {context.fields.get('PRIORITY')}"
    )


@then('the timestamp is "{timestamp}"')
def step_check_timestamp(context, timestamp):
    assert context.fields["TIMESTAMP"] == timestamp, (
        f"Expected timestamp {timestamp}, got {context.fields.get('TIMESTAMP')}"
    )


@then("syslog-ng receives a message with a timestamp within {seconds:d} seconds of now")
def step_check_timestamp_within(context, seconds):
    raw = context.fields["TIMESTAMP"]
    received = datetime.fromisoformat(raw).astimezone(timezone.utc)
    now = datetime.now(timezone.utc)
    delta = abs((now - received).total_seconds())
    assert delta <= seconds, (
        f"Timestamp {raw} is {delta:.1f}s from now, expected within {seconds}s"
    )


@then("syslog-ng receives a message with the system hostname")
def step_check_system_hostname(context):
    expected = socket.gethostname()
    assert context.fields["HOSTNAME"] == expected, (
        f"Expected hostname {expected}, got {context.fields.get('HOSTNAME')}"
    )


@then("syslog-ng receives a message with the process ID of the example program")
def step_check_example_pid(context):
    expected = str(context.example_pid)
    assert context.fields["PROCID"] == expected, (
        f"Expected PID {expected}, got {context.fields.get('PROCID')}"
    )


@then('the hostname is "{hostname}"')
def step_check_hostname(context, hostname):
    assert context.fields["HOSTNAME"] == hostname, (
        f"Expected hostname {hostname}, got {context.fields.get('HOSTNAME')}"
    )


@then('the app name is "{app_name}"')
def step_check_app_name(context, app_name):
    assert context.fields["APP_NAME"] == app_name, (
        f"Expected app name {app_name}, got {context.fields.get('APP_NAME')}"
    )


@then('the process ID is "{procid}"')
def step_check_procid(context, procid):
    assert context.fields["PROCID"] == procid, (
        f"Expected process ID {procid}, got {context.fields.get('PROCID')}"
    )


@then('the message ID is "{msgid}"')
def step_check_msgid(context, msgid):
    assert context.fields["MSGID"] == msgid, (
        f"Expected message ID {msgid}, got {context.fields.get('MSGID')}"
    )


@then('the message is "{msg}"')
def step_check_msg(context, msg):
    assert context.fields["MSG"] == msg, (
        f"Expected message {msg}, got {context.fields.get('MSG')}"
    )


@then("syslog-ng receives {count:d} message")
@then("syslog-ng receives {count:d} messages")
def step_check_message_count(context, count):
    # For interactive processes, refresh the line count
    if hasattr(context, "interactive_process"):
        wait_for_messages(context, count)
    assert context.message_count == count, (
        f"Expected {count} messages, got {context.message_count}"
    )


@then("syslog-ng receives no more messages")
def step_check_no_more_messages(context):
    before = oracle_record_count(context.received_log, context.oracle_format)
    time.sleep(5)
    after = oracle_record_count(context.received_log, context.oracle_format)
    assert after == before, (
        f"Expected no more messages, but received {after - before} additional"
    )


@then('the structured data contains sequenceId "{value}"')
def step_check_sequence_id(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    match = re.search(r'sequenceId="(\d+)"', sd)
    assert match, (
        f"No sequenceId found in structured data: {sd}"
    )
    assert match.group(1) == value, (
        f"Expected sequenceId {value}, got {match.group(1)}"
    )


@then('the structured data contains tzKnown "{value}"')
def step_check_tz_known(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    match = re.search(r'tzKnown="(\d+)"', sd)
    assert match, (
        f"No tzKnown found in structured data: {sd}"
    )
    assert match.group(1) == value, (
        f"Expected tzKnown {value}, got {match.group(1)}"
    )


@then('the structured data contains isSynced "{value}"')
def step_check_is_synced(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    match = re.search(r'isSynced="(\d+)"', sd)
    assert match, (
        f"No isSynced found in structured data: {sd}"
    )
    assert match.group(1) == value, (
        f"Expected isSynced {value}, got {match.group(1)}"
    )


@then('the structured data contains software "{value}"')
def step_check_software(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    match = re.search(r'software="([^"]*)"', sd)
    assert match, (
        f"No software found in structured data: {sd}"
    )
    assert match.group(1) == value, (
        f"Expected software {value}, got {match.group(1)}"
    )


@then('the structured data contains swVersion "{value}"')
def step_check_sw_version(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    match = re.search(r'swVersion="([^"]*)"', sd)
    assert match, (
        f"No swVersion found in structured data: {sd}"
    )
    assert match.group(1) == value, (
        f"Expected swVersion {value}, got {match.group(1)}"
    )


@then('the structured data contains enterpriseId "{value}"')
def step_check_enterprise_id(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    # SD-PARAM-VALUE escapes ", \ and ] with a backslash (RFC 5424 §6.3.3),
    # so the matcher must accept escape sequences inside the value rather
    # than terminating at the first inner quote.
    match = re.search(r'enterpriseId="((?:\\.|[^"])*)"', sd)
    assert match, (
        f"No enterpriseId found in structured data: {sd}"
    )
    actual = re.sub(r"\\(.)", r"\1", match.group(1))
    assert actual == value, (
        f"Expected enterpriseId {value}, got {actual}"
    )


@then('the structured data contains ip "{value}"')
def step_check_ip(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    matches = re.findall(r'ip="((?:\\.|[^"])*)"', sd)
    actuals = [re.sub(r"\\(.)", r"\1", m) for m in matches]
    assert value in actuals, (
        f"Expected ip {value} in structured data: {sd} (found ips: {actuals})"
    )


@then('the structured data contains language "{value}"')
def step_check_language(context, value):
    sd = context.fields.get("STRUCTURED_DATA", "")
    # SD-PARAM-VALUE escapes ", \ and ] with a backslash (RFC 5424 §6.3.3),
    # so the matcher must accept escape sequences inside the value rather
    # than terminating at the first inner quote.
    match = re.search(r'language="((?:\\.|[^"])*)"', sd)
    assert match, (
        f"No language found in structured data: {sd}"
    )
    actual = re.sub(r"\\(.)", r"\1", match.group(1))
    assert actual == value, (
        f"Expected language {value}, got {actual}"
    )


@then("the structured data contains sysUpTime as a decimal integer")
def step_check_sys_up_time_shape(context):
    sd = context.fields.get("STRUCTURED_DATA", "")
    match = re.search(r'sysUpTime="(\d+)"', sd)
    assert match, (
        f"No sysUpTime found in structured data: {sd}"
    )
    # Live boot clocks (CLOCK_BOOTTIME / GetTickCount64) always read > 0 by the
    # time the example program runs. Pinning > 0 catches a regression where the
    # implementation collapses to a constant zero.
    assert int(match.group(1)) > 0, (
        f"Expected positive sysUpTime, got {match.group(1)} in structured data: {sd}"
    )


@then("syslog-ng receives {count:d} messages with sequential sequenceId values")
def step_check_sequential_ids(context, count):
    assert context.message_count == count, (
        f"Expected {count} messages, got {context.message_count}"
    )
    for i, line in enumerate(context.all_lines, start=1):
        fields = parse_oracle_line(line, context.oracle_format)
        sd = fields.get("STRUCTURED_DATA", "")
        match = re.search(r'sequenceId="(\d+)"', sd)
        assert match, (
            f"Message {i}: no sequenceId in structured data: {sd}"
        )
        assert match.group(1) == str(i), (
            f"Message {i}: expected sequenceId {i}, got {match.group(1)}"
        )


@when("the client sends {count:d} messages")
def step_client_sends_n_messages(context, count):
    send_command(context.interactive_process, f"send {count}")
    # Allow time for the service thread to drain the buffer
    time.sleep(0.5)


@when("the client attempts to send it exits with code {code:d}")
def step_client_attempts_send_exits(context, code):
    process = context.interactive_process
    # Allow the service thread to drain the buffer and fill the store
    time.sleep(3)
    try:
        process.stdin.write("send\n")
        process.stdin.flush()
    except (BrokenPipeError, OSError):
        pass
    process.wait(timeout=10)
    assert process.returncode == code, (
        f"Expected exit code {code}, got {process.returncode}"
    )
    del context.interactive_process


@when("the client is killed")
def step_client_is_killed(context):
    context.interactive_process.send_signal(signal.SIGKILL)
    context.interactive_process.wait(timeout=5)
    del context.interactive_process


@then("the messages have contiguous sequenceIds")
def step_check_contiguous_sequence_ids(context):
    ids = []
    for line in context.all_lines:
        fields = parse_syslog_ng_line(line)
        sd = fields.get("STRUCTURED_DATA", "")
        match = re.search(r'sequenceId="(\d+)"', sd)
        assert match, (
            f"No sequenceId in structured data: {sd}"
        )
        ids.append(int(match.group(1)))
    # Check that IDs form a contiguous ascending sequence
    for i in range(1, len(ids)):
        assert ids[i] == ids[i - 1] + 1, (
            f"Non-contiguous sequenceIds: {ids[i - 1]} followed by {ids[i]}"
        )


@then("the last {count:d} messages have contiguous sequenceIds starting from {start:d}")
def step_check_last_n_contiguous_ids(context, count, start):
    assert len(context.all_lines) >= count, (
        f"Expected at least {count} messages, got {len(context.all_lines)}"
    )
    last_n = context.all_lines[-count:]
    for i, line in enumerate(last_n):
        fields = parse_syslog_ng_line(line)
        sd = fields.get("STRUCTURED_DATA", "")
        match = re.search(r'sequenceId="(\d+)"', sd)
        assert match, (
            f"Message {i + 1} of last {count}: no sequenceId in structured data: {sd}"
        )
        expected = start + i
        actual = int(match.group(1))
        assert actual == expected, (
            f"Message {i + 1} of last {count}: expected sequenceId {expected}, "
            f"got {actual}"
        )


@then("the replayed messages have sequenceIds {id_list}")
def step_check_replayed_sequence_ids(context, id_list):
    expected = [int(x.strip()) for x in id_list.split(",")]
    assert len(context.all_lines) >= len(expected), (
        f"Expected at least {len(expected)} messages, "
        f"got {len(context.all_lines)}"
    )
    # Replayed messages are the most recent batch excluding the first message
    # from the previous session (already verified)
    replayed = context.all_lines[-len(expected):]
    for i, line in enumerate(replayed):
        fields = parse_syslog_ng_line(line)
        sd = fields.get("STRUCTURED_DATA", "")
        match = re.search(r'sequenceId="(\d+)"', sd)
        assert match, (
            f"Replayed message {i + 1}: no sequenceId in structured data: {sd}"
        )
        actual = int(match.group(1))
        assert actual == expected[i], (
            f"Replayed message {i + 1}: expected sequenceId {expected[i]}, "
            f"got {actual}"
        )


@then("the last message has sequenceId {value:d}")
def step_check_last_sequence_id(context, value):
    assert context.all_lines, "No messages received to check last sequenceId"
    fields = parse_syslog_ng_line(context.all_lines[-1])
    sd = fields.get("STRUCTURED_DATA", "")
    match = re.search(r'sequenceId="(\d+)"', sd)
    assert match, (
        f"No sequenceId in last message structured data: {sd}"
    )
    actual = int(match.group(1))
    assert actual == value, (
        f"Last message: expected sequenceId {value}, got {actual}"
    )


# --- S20.1 SwitchingSender steps ------------------------------------------


@given("the switching example is running with default transport {transport:w}")
def step_switching_example_running(context, transport):
    cmd = build_threaded_command(context, transport)
    start_threaded_example(context, cmd)


@when("the client switches to transport {transport:w}")
def step_client_switches_transport(context, transport):
    send_command(context.interactive_process, f"switch {transport}")
    # Let any in-flight messages drain before subsequent sends assume the new
    # selector is live. Matches the settle pattern used after `send`.
    time.sleep(0.2)


def wait_for_per_transport_messages(context, transport, expected):
    """Wait for `expected` new lines to appear in the per-transport oracle."""
    path = PER_TRANSPORT_LOG[transport]
    baseline = context.lines_before_per_transport.get(transport, 0)
    deadline = time.monotonic() + 5
    while line_count(path) - baseline < expected:
        if time.monotonic() > deadline:
            actual = line_count(path) - baseline
            raise AssertionError(
                f"{path} received {actual} of {expected} messages within 5 seconds"
            )
        time.sleep(0.1)


@then("syslog-ng receives {count:d} message over {transport:w}")
@then("syslog-ng receives {count:d} messages over {transport:w}")
def step_check_per_transport_count(context, count, transport):
    wait_for_per_transport_messages(context, transport, count)
    path = PER_TRANSPORT_LOG[transport]
    baseline = context.lines_before_per_transport.get(transport, 0)
    actual = line_count(path) - baseline
    assert actual == count, (
        f"Expected {count} {transport} message(s), got {actual} in {path}"
    )
