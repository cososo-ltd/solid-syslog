@tcp @windows_wip
Feature: TCP message delivery (single-task example)
  The Linux single-task example (NullBuffer, synchronous send) sends
  messages via TCP transport with RFC 6587 octet-counting framing.
  Linux-only post-S13.18 — Windows TCP coverage now comes from
  tcp_transport.feature via the buffered example. The single-task
  binary remains valuable as a bare-metal model on Linux: it pins
  the synchronous-send path with no service thread.

  Scenario: Single-task message delivered over TCP
    Given syslog-ng is running
    When the example program sends a syslog message with transport tcp
    Then syslog-ng receives a message with priority "134"
