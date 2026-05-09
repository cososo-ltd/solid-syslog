@udp
@freertoswip
Feature: Structured data — meta SD-ELEMENT
  The library populates the IANA-registered "meta" SD-ELEMENT with
  sequenceId, sysUpTime, and language per RFC 5424 §7.3.

  Scenario: First message has sequence ID 1
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains sequenceId "1"

  Scenario: Sequence ID increments with each message
    Given the syslog oracle is running
    When the example program sends 3 syslog messages
    Then the syslog oracle receives 3 messages with sequential sequenceId values

  Scenario: Message includes sysUpTime and language from example callbacks
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains sysUpTime as a decimal integer
    And the structured data contains language "en-GB"
