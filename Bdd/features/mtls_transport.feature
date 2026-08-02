@mtls
Feature: Mutual TLS message delivery
  The BDD target authenticates itself to the oracle with a client
  certificate over RFC 5425 TLS, exercising mTLS end-to-end. Satisfies
  IEC 62443 CR 2.12 (non-repudiation, SL3+) — the SIEM cryptographically
  identifies the sender. Cross-platform: Linux runner uses syslog-ng,
  Windows runner uses otelcol-contrib with client_ca_file.

  Scenario: Message delivered over mutual TLS
    Given the syslog oracle is running
    When the BDD target sends a syslog message with transport mtls
    Then the syslog oracle receives 1 message over mtls
    And the syslog oracle receives a message with priority "134"
