@tcp @buffered
Feature: TCP reconnection after server outage
  The TCP sender detects connection failure and reconnects when the
  server recovers. With no store, Service holds the record whose send
  failed and reads nothing more from the buffer until it has gone, so
  the records logged during the outage wait in the buffer and arrive,
  in order, once the server is back.

  Scenario: Messages delivered after server recovery
    Given the syslog oracle is running
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the syslog oracle resumes accepting TCP connections
    And the client sends a message
    Then the syslog oracle receives 5 messages
    And the messages have contiguous sequenceIds
