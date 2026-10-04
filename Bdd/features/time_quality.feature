@udp
Feature: Structured data — time quality
  The library includes time quality metadata in structured data. RTC-equipped
  targets (Linux, Windows) report tzKnown="1" and isSynced="1"; embedded
  targets without an RTC report tzKnown="0" and isSynced="0", per the
  no-RTC product stance modelled in the FreeRTOS reference example.

  @rtc
  Scenario: Time quality appears in structured data
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains tzKnown "1"
    And the structured data contains isSynced "1"

  @rtc
  Scenario: Time quality and sequence ID coexist
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains sequenceId "1"
    And the structured data contains tzKnown "1"
    And the structured data contains isSynced "1"

  @no_rtc
  Scenario: Time quality reflects no RTC
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains tzKnown "0"
    And the structured data contains isSynced "0"

  @no_rtc
  Scenario: Time quality and sequence ID coexist (no RTC)
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains sequenceId "1"
    And the structured data contains tzKnown "0"
    And the structured data contains isSynced "0"
