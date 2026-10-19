@tcp @buffered @host_buffer
Feature: Buffer discard policy
  With a store that keeps nothing, the buffer is the only queue. Service holds
  the record whose send failed and reads nothing more until it has gone, so the
  records logged during an outage fill the buffer behind it. The discard policy
  the logger was given decides what becomes of the records that do not fit, and
  of the record Service holds once the buffer has filled behind it.

  Each scenario logs one record into the outage first, so Service is holding it
  before the burst arrives and the burst meets a buffer nothing is read from.
  The message body is padded so a hundred records outgrow the buffer on every
  hosted target; the Linux queue holds ten records whatever their size.

  Background:
    Given the syslog oracle is running
    And the client's message body is 250 characters long

  Scenario: Discard-newest keeps the records that fitted and refuses the rest
    Given the discard policy is newest
    And the BDD target is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends a message
    And the client sends 100 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle finishes draining
    And the syslog oracle received sequenceId 2
    And the syslog oracle did not receive sequenceId 102
    And the outage messages have contiguous sequenceIds
    And the target reports once that the buffer lost records

  Scenario: Discard-oldest keeps the latest records
    Given the discard policy is oldest
    And the BDD target is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends a message
    And the client sends 100 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle finishes draining
    And the syslog oracle received sequenceId 102
    And the syslog oracle did not receive sequenceId 2
    And the target reports that the buffer lost records

  Scenario: Halt stops the application when the buffer overflows
    Given the discard policy is halt
    And the halt callback exits the process
    And the BDD target is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends a message
    And the client sends 100 messages
    And the client attempts to send it exits with code 2
    Then the target reports once that the buffer lost records

  Scenario: Halt prevents further service after the buffer overflows
    Given the discard policy is halt
    And the BDD target is running with transport tcp and no structured data
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends a message
    And the client sends 100 messages
    And the syslog oracle resumes accepting TCP connections
    Then the syslog oracle receives no more messages
    And the target reports once that the buffer lost records
