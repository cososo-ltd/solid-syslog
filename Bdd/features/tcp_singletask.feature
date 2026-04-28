@tcp
Feature: TCP message delivery (single-task example)
  The single-task example sends messages via TCP transport with RFC
  6587 octet-counting framing. Companion to the @buffered
  tcp_transport.feature (which exercises the threaded example) — this
  one is the runner-agnostic version: it runs on bdd-linux-syslog-ng
  via syslog-ng's TCP listener and on bdd-windows-otel via the OTel
  Collector's TCP receiver. Together they validate Windows TCP
  end-to-end (S13.10).

  Scenario: Single-task message delivered over TCP
    Given syslog-ng is running
    When the example program sends a syslog message with transport tcp
    Then syslog-ng receives a message with priority "134"
