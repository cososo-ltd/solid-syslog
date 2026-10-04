@udp
Feature: Walking skeleton end-to-end
  The BDD target sends an RFC 5424 message via UDP.
  syslog-ng receives it and writes the parsed fields to a log file.

  Scenario: SolidSyslog sends a valid RFC 5424 message to syslog-ng
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the syslog oracle receives a message with priority "134"
    And the syslog oracle receives a message with a timestamp within 60 seconds of now
    And the syslog oracle receives a message with the system hostname
    And the app name is "SolidSyslogBddTarget"
    And the syslog oracle receives a message with the process ID of the BDD target
