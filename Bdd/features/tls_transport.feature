@tls
Feature: TLS message delivery
  The buffered example sends messages via TLS transport (RFC 5425) —
  RFC 6587 octet-counting framing over TLS — to the configured oracle
  (syslog-ng on Linux, otelcol-contrib on Windows; same listener cert).

  Scenario: Message delivered over TLS
    Given syslog-ng is running
    When the buffered example sends a syslog message with transport tls
    Then syslog-ng receives 1 message over tls
    And syslog-ng receives a message with priority "134"

  @tls13
  Scenario: Library negotiates TLS 1.3 against a server that requires it
    # syslog-ng's TLS receivers in syslog-ng-full.conf disable all protocols
    # below 1.3 via ssl-options, so message delivery here proves the handshake
    # completed at TLS 1.3 — the library has no 1.3-specific knob, it just
    # happens because OpenSSL prefers the highest mutually-supported version.
    Given syslog-ng is running
    When the buffered example sends a syslog message with transport tls
    Then syslog-ng receives 1 message over tls
