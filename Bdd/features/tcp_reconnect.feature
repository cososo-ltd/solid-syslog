@tcp @buffered
Feature: TCP reconnection after server outage
  The TCP sender detects connection failure and reconnects when the
  server recovers. Messages sent during the outage are lost (store
  and forward is a separate feature).

  Scenario: Message delivered after server recovery
    Given the syslog oracle is running
    And the threaded example is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends a message
    And the syslog oracle resumes accepting TCP connections
    And the client sends a message
    Then the syslog oracle receives 2 messages
