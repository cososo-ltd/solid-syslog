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

  @host_store
  Scenario: Messages delivered after a stored record is corrupted
    Given the syslog oracle is running
    And the block store is enabled
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the client is killed
    And a byte of the first stored record still to send is corrupted
    And the syslog oracle resumes accepting TCP connections
    Given the BDD target is running with transport tcp
    Then the syslog oracle receives 3 messages
    And the replayed messages have sequenceIds 3, 4
    And the target reports that a stored record was discarded

  # A record the policy cannot open for want of its key is not taken for
  # tampering: it waits in the store, with the records behind it, until the key
  # returns. @hmac and @aesgcm run wherever those policies are wired.
  @hmac
  Scenario: Stored messages wait for an HMAC key that is briefly unavailable
    Given the syslog oracle is running
    And the block store is enabled
    And the security policy is hmac-sha256
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the key is withheld
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle receives no more messages
    And the target reports once that the key is unavailable
    When the key is restored
    Then the syslog oracle receives 4 messages
    And the messages have contiguous sequenceIds

  @aesgcm
  Scenario: Stored messages wait for an AES-256-GCM key that is briefly unavailable
    Given the syslog oracle is running
    And the block store is enabled
    And the security policy is aes-256-gcm
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the key is withheld
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle receives no more messages
    And the target reports once that the key is unavailable
    When the key is restored
    Then the syslog oracle receives 4 messages
    And the messages have contiguous sequenceIds

  @host_store
  Scenario: Messages delivered after the store freezes
    Given the syslog oracle is running
    And the block store is enabled on a path that cannot be opened
    And the BDD target is running with transport tcp
    When the client sends 3 messages
    Then the syslog oracle receives 3 messages
    And the target reports that the store froze
