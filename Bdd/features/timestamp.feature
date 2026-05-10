@udp
@rtc
Feature: Timestamp encoding
  The library captures the timestamp at raise-time via an injected clock
  and formats it as RFC 5424 FULL-DATE "T" FULL-TIME. The no-RTC product
  shape (config.clock = NULL -> NILVALUE TIMESTAMP per RFC 5424 §6.2.3.1)
  is covered by formatter unit tests; syslog-ng silently substitutes
  receipt time for ${ISODATE} and ${S_ISODATE} when the wire TIMESTAMP
  is NILVALUE, so a BDD assertion against the oracle would re-test what
  the unit tests already cover, with no extra coverage.

  Scenario: Timestamp is received by syslog-ng
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the syslog oracle receives a message with a timestamp within 5 seconds of now
