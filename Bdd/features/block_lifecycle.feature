@tcp @buffered
Feature: Block lifecycle
  The block store rotates blocks across files of fixed size. Once every record
  in a non-active block is sent and acknowledged, that block is disposed
  immediately — flash drivers prefer "erase-then-write" so producers can drain
  retired blocks during idle moments rather than clustering erases at capacity
  boundaries. The active write block is never disposed by this path.

  Scenario: Active write block is not disposed when its only records are sent
    Given syslog-ng is running
    And the block store is enabled with max-blocks 4 and max-block-size 5000 and discard-policy oldest
    And the threaded example is running with transport tcp and no structured data
    And the set of existing block files is recorded
    When the client sends 2 messages
    Then syslog-ng receives 2 messages
    And no recorded block file has been disposed

  Scenario: Older block is disposed once all its records are drained
    Given syslog-ng is running
    And the block store is enabled with max-blocks 4 and max-block-size 520 and discard-policy oldest
    And the threaded example is running with transport tcp and no structured data
    When the client sends 5 messages
    Then syslog-ng receives 5 messages
    And block file 00 has been disposed
