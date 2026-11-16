# SF020 observed local evidence

Baseline3/3; final Release4/4, Debug4/4, ASan+UBSan2/2. These are focused tests,
not full legacy-suite results. Exact logs retained. native-observations.txt contains
independent double recurrence error, actual path onsets, buffer bytes, decay,
control step, fixed-control spur and one-wire/modal measurements.

arm.json is the actual pinned Cortex-M7 object/layout/call/static-frame report.
It does not record a firmware link, measured cycles or physical execution.
host-performance.json records three full-process Release wall-time runs; includes
three diagnostic output stores, excludes file IO, uncontrolled clocks/scheduling.
The source paths in tool receipts identify this workspace, not required locations.

verification.json records452 existing source/header/firmware/fixture hashes kept
unchanged, original CI steps retained, fixture regeneration and listening hashes.
Remote Actions execution is reported separately in the PR after publication.
