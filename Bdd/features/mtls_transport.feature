@mtls
Feature: Mutual TLS message delivery
  The buffered example authenticates itself to the oracle with a client
  certificate over RFC 5425 TLS, exercising mTLS end-to-end. Satisfies
  IEC 62443 SL4 CR 2.12 (non-repudiation) — the SIEM cryptographically
  identifies the sender. Cross-platform: Linux runner uses syslog-ng,
  Windows runner uses otelcol-contrib with client_ca_file.

  Scenario: Message delivered over mutual TLS
    Given syslog-ng is running
    When the buffered example sends a syslog message with transport mtls
    Then syslog-ng receives 1 message over mtls
    And syslog-ng receives a message with priority "134"
