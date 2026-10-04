@udp
Feature: Message header fields
  The library includes hostname, app-name, and process ID in the
  RFC 5424 message header.

  Scenario: Hostname matches the system hostname
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the syslog oracle receives a message with the system hostname

  Scenario: App name matches the BDD target
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the app name is "SolidSyslogBddTarget"

  Scenario: Process ID matches the BDD target PID
    Given the syslog oracle is running
    When the BDD target sends a syslog message
    Then the syslog oracle receives a message with the process ID of the BDD target
