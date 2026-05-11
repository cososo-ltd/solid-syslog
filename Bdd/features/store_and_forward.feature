@tcp @buffered @store
Feature: Store and forward during sender outage
  When the syslog oracle goes down, messages accumulate in the
  file-based store. Once the oracle recovers, the service loop
  drains the store and delivers the buffered messages.

  Scenario: Messages delivered after sender outage
    Given the syslog oracle is running
    And the block store is enabled
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 5 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle receives 6 messages
    And the messages have contiguous sequenceIds
