@tcp
Feature: TCP message delivery
  An example wired with a real (non-Null) buffer sends messages via
  TCP transport with RFC 6587 octet-counting framing. Linux runner
  uses the pthread-driven Threaded example; Windows runner uses the
  Win32-thread-driven example (S13.18). The same scenario pins both
  wirings against their respective oracle (syslog-ng / OTel collector).

  Scenario: Message delivered over TCP
    Given syslog-ng is running
    When the buffered example sends a syslog message with transport tcp
    Then syslog-ng receives a message with priority "134"
