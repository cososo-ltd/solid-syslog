@udp
Feature: UDP datagram path-MTU clipping
  When a UDP message exceeds the path MTU the sender clips it to the
  path-MTU's safe payload, walking back over any partial UTF-8
  codepoint at the trim point so the receiver always sees valid UTF-8.

  The oversize scenario is POSIX-only (tagged windows_wip): the
  Windows BDD runner uses the OTel Collector on 127.0.0.1, and
  loopback's ~65535-byte MTU never triggers WSAEMSGSIZE for any
  message inside SOLIDSYSLOG_MAX_MESSAGE_SIZE, so the EMSGSIZE retry
  path can't actually fire. Full delivery runs on both runners now
  that S13.17 (#221) embedded a UTF-8 activeCodePage manifest in
  SolidSyslogExample.exe so multi-byte argv survives the MSVC CRT.

  Scenario: Full delivery of a UTF-8 message within the path MTU
    Given the syslog oracle is running
    When the example program sends a UTF-8 message that fits the path MTU
    Then the received message is byte-identical to the sent message

  @windows_wip
  @freertoswip
  Scenario: Oversize UTF-8 message is clipped at a codepoint boundary
    Given the syslog oracle is running
    When the example program sends an oversize UTF-8 message
    Then the received message is shorter than the sent message
    And the received message is a clean prefix of the sent message
