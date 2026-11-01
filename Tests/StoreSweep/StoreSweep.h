#ifndef STORESWEEP_H
#define STORESWEEP_H

#include <cstddef>
#include <string>

/* Cases per fault class, from SOLIDSYSLOG_STORE_SWEEP_BUDGET: a smoke budget
 * when unset, every case when 0. */
size_t StoreSweep_Budget();

/* The one case to run, from SOLIDSYSLOG_STORE_SWEEP_CASE as a failure printed
 * it; empty runs the budgeted sweep. */
std::string StoreSweep_CaseFilter();

#endif /* STORESWEEP_H */
