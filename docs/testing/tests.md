# Tests

What CI ran in the build that produced this site, down to each test and how
long it took. Every lane that runs tests leaves its results behind, and the
report below is generated from them: unit tests under each compiler and
sanitiser, integration tests against real upstreams, and the BDD scenarios
against every target, each organised by lane, executable and suite, with a
view across lanes for the question "where is this tested".

<!-- test-summary -->

## Reading the report

The report drills down. The overview leads with failures, then each kind of
testing with its lanes. A kind page holds the view across its lanes: for unit
and integration testing, which lanes ran each suite and how many of its tests;
for BDD, every feature as its file describes it, with every scenario against
every target. A lane is one CI job, named by what it does and what it
exercises; the [CI pipeline](../ci.md#jobs) describes each.

A unit or integration lane page lists its executables, each executable its
suites, and each suite opens to its tests with status, duration and a link to
the test's source at the commit that ran. A BDD lane page is the living
documentation of that target: each feature's prose, its scenarios with their
tags and outcome, and each scenario opening to its steps as they ran, with the
time each took. The Skipped page says what a skip means and which tags decided
each one.

Failures have a page of their own with the message each left. A pack page
gathers the tests that exercise one pack, attributed by the test file that
holds them, lane by lane. The overview says how test time is counted.

To run the same tests locally, build the `debug` preset's `junit` target as
[Building & testing](../builds.md) describes; the BDD scenarios are run as
[BDD testing](../bdd.md) describes.
