@tcp @buffered
Feature: Block lifecycle
  The block store rotates blocks across files of fixed size. Block files are
  disposed (deleted) only when capacity pressure forces rotation past the
  oldest block; idle drains do not delete blocks. This baseline pins today's
  semantics — S18.03 will introduce dispose-on-empty for non-active blocks.

  Scenario: Block files are not disposed when capacity is not exhausted
    Given syslog-ng is running
    And the block store is enabled with max-blocks 4 and max-block-size 5000 and discard-policy oldest
    And the threaded example is running with transport tcp and no structured data
    And the set of existing block files is recorded
    When the client sends 2 messages
    Then syslog-ng receives 2 messages
    And no recorded block file has been disposed
