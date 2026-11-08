# Tests

What CI ran in the build that produced this site, down to each test and how
long it took. Every lane that runs tests leaves its results behind, and the
report below is generated from them: unit tests under each compiler and
sanitiser, integration tests against real upstreams, and the BDD scenarios
against every target, each organised by lane, executable and suite, with a
view across lanes for the question "where is this tested".

<!-- test-summary -->

## Reading the report

A lane is one CI job, named by what it does and what it exercises; the
[CI pipeline](../ci.md#jobs) describes each. A lane page lists its executables,
each executable its suites with counts and time, and each suite opens to its
tests with status, duration and a link to the test's source at the commit that
ran. Failures come first on the overview and have a page of their own with the
message each left.

The two matrices answer where a thing is tested. Suites across lanes shows, for
each unit suite, which lanes ran it and how many of its tests. Scenarios across
lanes shows every BDD scenario against every target, with its tags; a skip is a
target whose tag filter excluded the scenario, or a `@requires_` tag naming a
tunable the target is built below, so a skip there is a decision, not a gap.

A pack page gathers the tests that exercise one pack, attributed by the test
file that holds them, lane by lane. Test time is the sum of every test's own
duration; lanes run in parallel, so it is not the run's wall clock, and
CppUTest reports to the millisecond.

To run the same tests locally, build the `debug` preset's `junit` target as
[Building & testing](../builds.md) describes; the BDD scenarios are run as
[BDD testing](../bdd.md) describes.
