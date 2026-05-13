@tcp @buffered @store @freertoswip
Feature: Capacity threshold alert
  # @freertoswip excludes this from the FreeRTOS BDD run (S08.05+).
  # Threshold-callback support on FreeRTOS is a follow-up — the wiring
  # in main.c plumbs g_pendingCapacityThreshold through to BlockStore
  # but the .feature relies on the harness inspecting a marker file
  # which has no semihosting equivalent today.
  an early-warning callback fires when the block store crosses a configured
  capacity threshold, before the terminal full-store callback engages.

  Scenario: Threshold callback fires when usage crosses configured threshold
    Given the syslog oracle is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy oldest
    And the capacity threshold callback is enabled at 200 bytes
    And the BDD target is running with transport tcp and no structured data
    When the syslog oracle stops accepting TCP connections
    And the client sends 4 messages
    Then the capacity threshold callback was invoked

  Scenario: Threshold callback does not fire while usage stays below threshold
    Given the syslog oracle is running
    And the block store is enabled with max-blocks 2 and max-block-size 520 and discard-policy oldest
    And the capacity threshold callback is enabled at 5000 bytes
    And the BDD target is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    And the capacity threshold callback was not invoked
