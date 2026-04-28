@tcp @buffered
Feature: Store capacity limit and discard policy
  The file store uses rotating files with a configurable capacity.
  When the store is full, the discard policy determines whether
  the oldest or newest messages are dropped.

  Scenario: Discard-oldest drops oldest messages when store overflows
    Given syslog-ng is running
    And the file store is enabled with max-files 7 and max-file-size 2055 and discard-policy oldest
    And each message is large enough to fill one store file
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then syslog-ng receives 1 message
    When the syslog server stops accepting TCP connections
    And the client sends 10 messages
    And the syslog server resumes accepting TCP connections
    Then syslog-ng receives 8 messages
    And the last 7 messages have contiguous sequenceIds starting from 5

  Scenario: Discard-newest preserves oldest messages when store overflows
    Given syslog-ng is running
    And the file store is enabled with max-files 7 and max-file-size 2055 and discard-policy newest
    And each message is large enough to fill one store file
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then syslog-ng receives 1 message
    When the syslog server stops accepting TCP connections
    And the client sends 10 messages
    And the syslog server resumes accepting TCP connections
    Then syslog-ng receives 8 messages
    And the last 7 messages have contiguous sequenceIds starting from 2

  Scenario: Halt stops the application when store overflows
    Given syslog-ng is running
    And the file store is enabled with max-files 7 and max-file-size 2055 and discard-policy halt
    And each message is large enough to fill one store file
    And the halt callback exits the process
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then syslog-ng receives 1 message
    When the syslog server stops accepting TCP connections
    And the client sends 8 messages
    And the client attempts to send it exits with code 2

  Scenario: Halt prevents further service after store overflows
    Given syslog-ng is running
    And the file store is enabled with max-files 7 and max-file-size 2055 and discard-policy halt
    And each message is large enough to fill one store file
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then syslog-ng receives 1 message
    When the syslog server stops accepting TCP connections
    And the client sends 10 messages
    And the syslog server resumes accepting TCP connections
    Then syslog-ng receives no more messages
