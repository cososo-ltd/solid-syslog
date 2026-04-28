@udp
Feature: UDP datagram path-MTU clipping
  When a UDP message exceeds the path MTU the sender clips it to the
  path-MTU's safe payload, walking back over any partial UTF-8
  codepoint at the trim point so the receiver always sees valid UTF-8.

  Both scenarios are POSIX-only on the Windows BDD runner, but for
  different reasons:

    - Full delivery (windows_wip): SolidSyslogExample.exe receives its
      --message argv in the Windows active code page rather than UTF-8,
      so multi-byte UTF-8 bodies arrive at SolidSyslog_Log already
      mojibake'd and the formatter substitutes U+FFFD per Unicode §3.9.
      Tracked in S13.17 (#221); once that lands, drop this tag.
    - Oversize (windows_wip): the OTel runner is on 127.0.0.1 and
      loopback's ~65535-byte MTU never triggers WSAEMSGSIZE for any
      message inside SOLIDSYSLOG_MAX_MESSAGE_SIZE, so the EMSGSIZE
      retry path can't actually fire. This one stays POSIX-only by
      virtue of the Windows-loopback MTU.

  @windows_wip
  Scenario: Full delivery of a UTF-8 message within the path MTU
    Given syslog-ng is running
    When the example program sends a UTF-8 message that fits the path MTU
    Then the received message is byte-identical to the sent message

  @windows_wip
  Scenario: Oversize UTF-8 message is clipped at a codepoint boundary
    Given syslog-ng is running
    When the example program sends an oversize UTF-8 message
    Then the received message is shorter than the sent message
    And the received message is a clean prefix of the sent message
