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

  # Same flow, but records are sealed at rest with HMAC-SHA256 instead of CRC-16.
  # Proves the keyed policy is transparent to behaviour and that real HMAC
  # round-trips on the target: records sealed on write are verified on
  # replay-read after a power cycle. @hmac runs on every store-capable runner
  # that wires an HMAC policy — FreeRTOS-plustcp (mbedTLS, S17.02) plus Linux and
  # Windows (OpenSSL, S17.01). FreeRTOS-lwip is excluded via @store.
  @hmac
  Scenario: Stored messages replayed after power cycle with HMAC-SHA256 at rest
    Given the syslog oracle is running
    And the block store is enabled
    And the security policy is hmac-sha256
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
