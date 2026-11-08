# Coverage

What the unit tests exercised, line by line and branch by branch, in the build
that produced this site. Each pack links into its part of the report, where
every source file lists its lines with the count of times each ran and which
way each branch went. The table is computed from the tracefile the coverage
lane's gate was judged on, so the number shown is the number that passed; the
gate itself is described with the lane in the [CI pipeline](../ci.md#jobs).

<!-- coverage-table -->

## Reading the report

A pack's index lists its source files with their line and branch rates. A
file's page shows the source with each line's execution count in the margin.
A line that never ran is marked, and a branch taken one way only is marked at
the condition. The counts come from every test executable together, so a line
counts as covered whichever executable reached it.

The report covers Core and every platform pack a Linux host can build. The
Windows pack is tested on Windows and is not measured. The cross-built targets
run their unit tests on the host against fakes, and that is what is measured.

To measure locally, build the `coverage` preset as
[Building & testing](../builds.md#coverage---coverage) describes.
