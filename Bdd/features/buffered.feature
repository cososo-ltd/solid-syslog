@udp
@freertoswip
Feature: Buffered message delivery
  An example wired with a real (non-Null) buffer drives messages
  through a service thread that drains the buffer and sends to the
  oracle. Linux runner uses the pthread-driven Threaded example with
  PosixMessageQueueBuffer; Windows runner uses the Win32-thread-driven
  example with the portable CircularBuffer (S13.18). The same scenario
  pins both wirings.

  Scenario: Single buffered message arrives at the oracle
    Given the syslog oracle is running
    When the buffered example sends a syslog message
    Then the syslog oracle receives a message with priority "134"
    And the syslog oracle receives a message with a timestamp within 5 seconds of now

  Scenario: Multiple buffered messages arrive at the oracle
    Given the syslog oracle is running
    When the buffered example sends 3 syslog messages
    Then the syslog oracle receives 3 messages
