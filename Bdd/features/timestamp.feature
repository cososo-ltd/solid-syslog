@udp
@freertoswip
Feature: Timestamp encoding
  The library captures the timestamp at raise-time via an injected clock
  and formats it as RFC 5424 FULL-DATE "T" FULL-TIME.

  Scenario: Timestamp is received by syslog-ng
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the syslog oracle receives a message with a timestamp within 5 seconds of now
