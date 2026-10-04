@udp
Feature: Structured data — meta SD-ELEMENT
  The library populates the IANA-registered "meta" SD-ELEMENT with
  sequenceId, sysUpTime, and language per RFC 5424 §7.3.

  @vxworks64wip
  Scenario: First message has sequence ID 1
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains sequenceId "1"

  @vxworks64wip
  Scenario: Sequence ID increments with each message
    Given the syslog oracle is running
    When the BDD target sends 3 syslog messages
    Then the syslog oracle receives 3 messages with sequential sequenceId values

  @vxworks64wip
  Scenario: Message includes sysUpTime and language from example callbacks
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains sysUpTime as a decimal integer
    And the structured data contains language "en-GB"
