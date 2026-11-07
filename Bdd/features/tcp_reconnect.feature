@tcp
Feature: TCP reconnection after server outage
  The TCP sender detects connection failure and reconnects when the
  server recovers. With no store, what becomes of the records logged
  during the outage is the buffer's to decide. A queue keeps them:
  Service holds the record whose send failed and reads nothing more
  from the buffer until it has gone, so they wait and arrive, in order,
  once the server is back. A passthrough buffer has nowhere to keep
  them, so they are lost, and the logger reports the loss.

  @buffered
  Scenario: Messages delivered after server recovery
    Given the syslog oracle is running
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the syslog oracle resumes accepting TCP connections
    And the client sends a message
    Then the syslog oracle receives 5 messages
    And the messages have contiguous sequenceIds
    And the syslog oracle received a report that delivery failed
    And the syslog oracle received a report that delivery was restored
    And the target reports once that the collector could not be reached
    And the target reports once that delivery was restored

  @passthrough
  Scenario: Records sent during the outage are lost, and the loss is reported once the server is back
    # A passthrough buffer hands each record to the sender as it is logged,
    # so a record the server could not take is gone. The logger's report of
    # the outage is held instead, and lands with the first record the server
    # takes again, followed by the report that delivery was restored.
    Given the syslog oracle is running
    And the BDD target is running with transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the syslog oracle resumes accepting TCP connections
    And the client sends a message
    Then the syslog oracle receives 2 messages
    And the syslog oracle received a report that delivery failed
    And the syslog oracle received a report that delivery was restored
    And the target reports once that the collector could not be reached
    And the target reports once that delivery was restored
