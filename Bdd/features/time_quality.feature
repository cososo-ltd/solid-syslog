@udp
@freertoswip
Feature: Structured data — time quality
  The library includes time quality metadata in structured data.

  Scenario: Time quality appears in structured data
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains tzKnown "1"
    And the structured data contains isSynced "1"

  Scenario: Time quality and sequence ID coexist
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains sequenceId "1"
    And the structured data contains tzKnown "1"
