@tcp @buffered
Feature: Store and forward during sender outage
  When the syslog server goes down, messages accumulate in the
  file-based store. Once the server recovers, the service loop
  drains the store and delivers the buffered messages.

  Scenario: Messages delivered after sender outage
    Given syslog-ng is running
    And the block store is enabled
    And the threaded example is running with transport tcp
    When the client sends a message
    Then syslog-ng receives 1 message
    When the syslog server stops accepting TCP connections
    And the client sends 5 messages
    And the syslog server resumes accepting TCP connections
    Then syslog-ng receives 6 messages
    And the messages have contiguous sequenceIds
