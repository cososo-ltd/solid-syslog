# Coverage

What the unit tests exercised, line by line, branch by branch and condition by
condition, in the build that produced this site. Each pack links into its part
of the report, where every source file lists its lines with the count of times
each ran and which way each branch went. The table is computed from the files
the coverage lane's gate was judged on, so the number shown is the number that
passed; the gate itself is described with the lane in the
[CI pipeline](../ci.md#jobs).

<!-- coverage-table -->

## Reading the report

A pack's index lists its source files with their line and branch rates. A
file's page shows the source with each line's execution count in the margin.
A line that never ran is marked, and a branch taken one way only is marked at
the condition. The counts come from every test executable together, so a line
counts as covered whichever executable reached it.

The MC/DC column counts conditions, not decisions. A decision such as
`a && b` has two conditions, and a condition is covered when the tests include
a pair of runs in which it alone changed and the decision's outcome changed
with it, which is modified condition/decision coverage as DO-178C and
ISO 26262 define it. Branch coverage is satisfied once each decision has gone
both ways; MC/DC asks that each condition has been shown to matter on its
own. The MC/DC report lists every decision with its conditions and the test
vectors the run produced, and names the pair each condition lacks.

Three conditions are not covered, and will not be on this host. The TCP
streams of the Posix, lwIP Sockets and VxWorks 6.4 packs treat `EAGAIN` and
`EWOULDBLOCK` alike as a receive that would block. On Linux, where the tests
run, the two names are one value, so the second comparison can never be the
one that decides. The targets where they differ are the reason the second
comparison is there.

The report covers Core and every platform pack a Linux host can build. The
Windows pack is tested on Windows and is not measured. The cross-built targets
run their unit tests on the host against fakes, and that is what is measured.

To measure locally, build the `coverage` preset as
[Building & testing](../builds.md#coverage---coverage) describes.
