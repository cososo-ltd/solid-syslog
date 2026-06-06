@udp
Feature: Custom structured data — caller-supplied SD-ELEMENT
  An integrator can attach a custom SD-ELEMENT to a single log call via
  SolidSyslog_LogWithSd. The library frames it per RFC 5424 §6.3 and the
  element reaches the receiver alongside the per-instance SDs.

  Scenario: A custom SD-ELEMENT reaches the oracle
    Given the syslog oracle is running
    When the BDD target sends a custom syslog message
    Then the structured data contains detail "Hello World"
