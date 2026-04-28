@udp @windows_wip
Feature: UDP datagram path-MTU clipping
  When a UDP message exceeds the path MTU the sender clips it to the
  path-MTU's safe payload, walking back over any partial UTF-8
  codepoint at the trim point so the receiver always sees valid UTF-8.

  These scenarios run on the POSIX BDD runner only (tagged
  windows_wip). The Windows BDD runner uses the OTel Collector on
  127.0.0.1; loopback's ~65535-byte MTU never triggers WSAEMSGSIZE
  for the message sizes we can produce inside
  SOLIDSYSLOG_MAX_MESSAGE_SIZE, and the OTel syslog receiver
  interprets BOM-less UTF-8 MSG bytes as Latin-1 — RFC 5424 §6.4
  requires a BOM that this library does not yet emit. The BOM gap
  is tracked in S07.05 (#219); once that lands, the Latin-1
  mojibake clears and the windows_wip tag can be dropped on the
  full-delivery scenario. The oversize scenario remains POSIX-only
  by virtue of Windows loopback MTU.

  Scenario: Full delivery of a UTF-8 message within the path MTU
    Given syslog-ng is running
    When the example program sends a UTF-8 message that fits the path MTU
    Then the received message is byte-identical to the sent message

  Scenario: Oversize UTF-8 message is clipped at a codepoint boundary
    Given syslog-ng is running
    When the example program sends an oversize UTF-8 message
    Then the received message is shorter than the sent message
    And the received message is a clean prefix of the sent message
