@tcp @buffered
Feature: Power cycle replay from block store
  After a power cycle, unsent messages persisted in the block store
  are replayed before new messages. The collector sees old-session
  sequenceIds followed by a jump to 1 (restart signal).

  Scenario: Stored messages replayed after power cycle
    Given syslog-ng is running
    And the block store is enabled
    And the threaded example is running with transport tcp
    When the client sends a message
    Then syslog-ng receives 1 message
    When the syslog server stops accepting TCP connections
    And the client sends 3 messages
    And the client is killed
    And the syslog server resumes accepting TCP connections
    Given the threaded example is running with transport tcp
    Then syslog-ng receives 4 messages
    And the replayed messages have sequenceIds 2, 3, 4
    When the client sends a message
    Then syslog-ng receives 5 messages
    And the last message has sequenceId 1
