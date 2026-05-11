@tcp @buffered @store
Feature: Power cycle replay from block store
  After a power cycle, unsent messages persisted in the block store
  are replayed before new messages. The collector sees old-session
  sequenceIds followed by a jump to 1 (restart signal).

  Scenario: Stored messages replayed after power cycle
    Given the syslog oracle is running
    And the block store is enabled
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the client is killed
    And the syslog oracle resumes accepting TCP connections
    Given the BDD target is running with transport tcp
    Then the syslog oracle receives 4 messages
    And the replayed messages have sequenceIds 2, 3, 4
    When the client sends a message
    Then the syslog oracle receives 5 messages
    And the last message has sequenceId 1
