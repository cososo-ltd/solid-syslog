@udp
@freertoswip
Feature: Structured data — origin
  The library includes origin metadata identifying the software component.

  Scenario: Origin software appears in structured data
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains software "SolidSyslogExample"

  Scenario: Origin version appears in structured data
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains swVersion "0.7.0"

  Scenario: Origin enterpriseId appears in structured data
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains enterpriseId "1.3.6.1.4.1.99999"

  Scenario: Origin ip parameter appears in structured data
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains ip "192.0.2.1"

  Scenario: All standard structured data present
    Given the syslog oracle is running
    When the example program sends a syslog message
    Then the structured data contains sequenceId "1"
    And the structured data contains tzKnown "1"
    And the structured data contains software "SolidSyslogExample"
    And the structured data contains swVersion "0.7.0"
    And the structured data contains enterpriseId "1.3.6.1.4.1.99999"
    And the structured data contains ip "192.0.2.1"
