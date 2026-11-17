# Separate import-policy follow-up (proposal only)

Do not label every permissive license MIT. Current catalog permits MIT/BSD-2 only.
A separate narrowly reviewed policy ticket should add BSD-3-Clause,0BSD and
BSL-1.0 when each actual source/license is checked, with positive/negative fixture
coverage, required notices and BSD-3 nonendorsement obligations preserved.
No such allowlist expansion is made here. BSD-3 is not rejected as copyleft.

Basics root is MIT, but bundled stfx-library.h explicitly declares Boost Software
License. Its missing distinct bundled license text/root-license ambiguity merits
source/license review before import; do not silently apply the root MIT instead.
hilbert-iir declares0BSD; it and linear are umbrella CMake dependencies, not direct
reverb processing includes. Exact pins are in third_party/chamber/transitive-audit.json.
The selected full Cloud Seed core is MIT throughout and carries its original
notices. Other candidates stay external host research dependencies/audit records,
not selected runtime code or catalog import-policy exceptions.
