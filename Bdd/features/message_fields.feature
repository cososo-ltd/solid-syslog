@udp
Feature: Message ID and message body
  The library includes message ID and message body in the RFC 5424 message.

  Scenario: Message ID appears in the message
    Given the syslog oracle is running
    When the example program sends a message with message ID "ID47"
    Then the message ID is "ID47"

  Scenario: Message body appears in the message
    Given the syslog oracle is running
    When the example program sends a message with body "system started"
    Then the message is "system started"

  @freertoswip
  Scenario: Complete RFC 5424 message with all fields
    Given the syslog oracle is running
    When the example program sends a complete message with message ID "CONN" and body "session opened"
    Then the syslog oracle receives a message with priority "134"
    And the syslog oracle receives a message with a timestamp within 5 seconds of now
    And the syslog oracle receives a message with the system hostname
    And the app name is "SolidSyslogExample"
    And the syslog oracle receives a message with the process ID of the example program
    And the message ID is "CONN"
    And the message is "session opened"
