@buffered
Feature: Switch transport at runtime
  The BDD target always wraps its UDP and TCP senders in a
  SwitchingSender. The --transport CLI flag sets the initial selector
  value, and the interactive `switch` command updates it at runtime.

  @udp @tcp
  Scenario: Switch from UDP to TCP mid-run delivers via both
    Given the syslog oracle is running
    And the BDD target is running with default transport udp
    When the client sends a message
    Then the syslog oracle receives 1 message over udp
    When the client switches to transport tcp
    And the client sends a message
    Then the syslog oracle receives 1 message over tcp
    And the syslog oracle receives 1 message over udp

  @tls
  Scenario: Switch from TCP to TLS mid-run delivers via both reliable transports
    Given the syslog oracle is running
    And the BDD target is running with default transport tcp
    When the client sends a message
    Then the syslog oracle receives 1 message over tcp
    When the client switches to transport tls
    And the client sends a message
    Then the syslog oracle receives 1 message over tls
    And the syslog oracle receives 1 message over tcp
