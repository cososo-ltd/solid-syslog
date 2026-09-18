"""How long a step waits for something it expects to observe.

Every wait these budgets cover is a poll loop: it returns the moment its
condition holds, so the budget is reached only when the condition never does.
A generous one therefore costs a passing run nothing and only decides how long
a genuine failure takes to report. Budgets that were tight enough to expire on
a slow runner have flaked - see #834.

Three kinds of wait are deliberately not covered. Waiting for a process to
exit before killing it, where a larger number is a slower teardown rather
than a safer one. The settle window in `the syslog oracle finishes draining`,
which is quiescence rather than a condition. And the wait in `the syslog
oracle receives no message over ...`, which is asserting a negative, so its
budget is a judgement about how long to keep looking rather than headroom.

A plain module because both step modules and `environment.py` need it, and
one step module cannot import another without behave registering its steps
twice.
"""

CONDITION_TIMEOUT_SECONDS = 30
