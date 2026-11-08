# Coverage

What the unit tests exercised, line by line and branch by branch, as it stood
on `main` when this site was built. Each pack below links into its part of the
report, where every source file lists its lines with the count of times each
ran and which way each branch went.

The figures are measured by CI's coverage lane over every test executable the
build registers, across all fourteen packs together, by gcov through lcov. The
table is computed from the same tracefile the lane's gate was judged on, so the
number shown is the number that passed. The gate is 90% on lines and 90% on
branches over all packs together; the target is 100%. A tracefile with no
branch data fails the gate rather than skipping it.

<!-- coverage-table -->

## Reading the report

A pack's index lists its source files with their line and branch rates. A
file's page shows the source with each line's execution count in the margin:
a line that never ran is marked, and a branch that was taken one way only is
marked at the condition. The counts come from the whole run of every test
executable, so a line counts as covered whichever executable reached it.

What is covered here is Core and every platform pack a Linux host can build.
The Windows pack is built and tested on Windows and is not measured. The
cross-built targets run their tests on the host against fakes, which is what is
measured; the code that runs on the board under QEMU is the same source, built
for a different machine.

To measure locally, build the `coverage` preset as
[Building & testing](../builds.md#coverage---coverage) describes; the same
script prints the table above from the local tracefile.
