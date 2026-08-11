@udp
Feature: Structured data — origin
  The library includes origin metadata identifying the software component.

  Scenario: Origin software appears in structured data
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains software "SolidSyslogBddTarget"

  Scenario: Origin version appears in structured data
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains swVersion "0.7.0"

  Scenario: Origin enterpriseId appears in structured data
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains enterpriseId "32473"

  Scenario: Origin ip parameter appears in structured data
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains ip "192.0.2.1"

  @rtc
  Scenario: All standard structured data present
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains sequenceId "1"
    And the structured data contains tzKnown "1"
    And the structured data contains software "SolidSyslogBddTarget"
    And the structured data contains swVersion "0.7.0"
    And the structured data contains enterpriseId "32473"
    And the structured data contains ip "192.0.2.1"

  @no_rtc
  Scenario: All standard structured data present (no RTC)
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the structured data contains sequenceId "1"
    And the structured data contains tzKnown "0"
    And the structured data contains isSynced "0"
    And the structured data contains software "SolidSyslogBddTarget"
    And the structured data contains swVersion "0.7.0"
    And the structured data contains enterpriseId "32473"
    And the structured data contains ip "192.0.2.1"
