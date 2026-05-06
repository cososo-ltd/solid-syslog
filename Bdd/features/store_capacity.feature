@tcp @buffered
Feature: Store capacity limit and discard policy
  The block store uses rotating files with a configurable capacity.
  When the store is full, the discard policy determines whether
  the oldest or newest messages are dropped.

  Note: scenarios are tagged @windows_wip pending S12.14 (architecture
  refactor that decouples buffer drain from sender state). The tight-loop
  `client sends N messages` step fills the in-memory buffer faster than
  the service thread can drain it on Windows, so records never reach the
  store and the discard policy can't engage. Linux compose runner happens
  to schedule producer and service so the buffer stays small enough that
  the assertions pass; that is timing-dependent and will be re-tuned in
  the PR #275 finale once S12.14 is on main.

  @windows_wip
  Scenario: Discard-oldest drops oldest messages when store overflows
    Given the syslog oracle is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy oldest
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 10 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle finishes draining
    And the syslog oracle received sequenceId 1
    And the syslog oracle received sequenceId 11
    And the syslog oracle did not receive sequenceId 2
    And the outage messages have contiguous sequenceIds

  @windows_wip
  Scenario: Discard-newest preserves oldest messages when store overflows
    Given the syslog oracle is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy newest
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 10 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle finishes draining
    And the syslog oracle received sequenceId 1
    And the syslog oracle received sequenceId 2
    And the syslog oracle did not receive sequenceId 11
    And the outage messages have contiguous sequenceIds

  @windows_wip
  Scenario: Halt stops the application when store overflows
    Given the syslog oracle is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy halt
    And the halt callback exits the process
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 8 messages
    And the client attempts to send it exits with code 2

  @windows_wip
  Scenario: Halt prevents further service after store overflows
    Given the syslog oracle is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy halt
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 10 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle receives no more messages
