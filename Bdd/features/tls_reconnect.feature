@tls @buffered
Feature: TLS reconnection after server outage
  A TLS stream writes through the plain TCP stream it is given, so a peer
  that closes its end should be seen the same way over TLS as over TCP.
  The risk is a TLS library taking a record into its own buffering without
  reaching the transport, and so returning success for a record that the
  closed connection will never deliver. If none is lost, the records logged
  during the outage wait in the buffer and arrive, in order, once the server
  is back.

  Scenario: Messages delivered over TLS after server recovery
    Given the syslog oracle is running
    And the BDD target is running with transport tls
    When the client sends a message
    Then the syslog oracle receives 1 message over tls
    When the syslog oracle stops accepting TCP connections
    And the client sends 3 messages
    And the syslog oracle resumes accepting TCP connections
    And the client sends a message
    Then the syslog oracle receives 5 messages
    And the syslog oracle receives 5 messages over tls
    And the messages have contiguous sequenceIds
