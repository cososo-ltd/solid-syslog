@udp
Feature: PRIVAL encoding
  The library calculates RFC 5424 PRIVAL from facility and severity.
  The example program accepts --facility and --severity flags,
  defaulting to local0 (16) and info (6) when omitted.

  Scenario Outline: Valid facility and severity produce correct PRIVAL
    Given the syslog oracle is running
    When the example program sends a message with facility <facility> and severity <severity>
    Then the syslog oracle receives a message with priority "<prival>"

    Examples:
      | facility | severity | prival |
      | 0        | 0        | 0      |
      | 1        | 6        | 14     |
      | 16       | 6        | 134    |
      | 23       | 7        | 191    |

  Scenario: Out-of-range facility is reported as internal error
    Given the syslog oracle is running
    When the example program sends a message with facility 24 and severity 6
    Then the syslog oracle receives a message with priority "43"

  Scenario: Out-of-range severity is reported as internal error
    Given the syslog oracle is running
    When the example program sends a message with facility 16 and severity 8
    Then the syslog oracle receives a message with priority "43"
