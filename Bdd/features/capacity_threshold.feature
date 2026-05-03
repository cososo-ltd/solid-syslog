@tcp @buffered
Feature: Capacity threshold alert
  an early-warning callback fires when the block store crosses a configured
  capacity threshold, before the terminal full-store callback engages.

  Scenario: Threshold callback fires when usage crosses configured threshold
    Given syslog-ng is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy oldest
    And the capacity threshold callback is enabled at 200 bytes
    And the threaded example is running with transport tcp and no structured data
    When the syslog server stops accepting TCP connections
    And the client sends 4 messages
    Then the capacity threshold callback was invoked

  Scenario: Threshold callback does not fire while usage stays below threshold
    Given syslog-ng is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy oldest
    And the capacity threshold callback is enabled at 5000 bytes
    And the threaded example is running with transport tcp and no structured data
    When the client sends a message
    Then syslog-ng receives 1 message
    And the capacity threshold callback was not invoked
