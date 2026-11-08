# Codex kickoff: source-of-truth and initial assignment
Date: 2026-11-07

## Context
This repository is the **first native C++ Parable Core proof of architecture**, not a nearly finished pedal. The flagship idea is an auditable traditional-astrology-to-audio-patch correspondence compiler; the second composer is a direct virtual pedalboard editor. Both must produce the same validated patch format, ultimately rendered in embedded C++ on Daisy Seed3. Read `PRODUCT_CONSTITUTION.md`, `AGENTS.md`, and `README.md` before proceeding.

### Preserved decisions
- Pilot target: 2027-04-01, go/no-go for a small pilot, not a guaranteed full release.
- Physical product: 4 locally stored and selectable complete presets, Register, master Volume, bypass; fully playable without phone.
- Phone editor: visual virtual pedalboard assembly + chart composer; phone is optional for performance.
- Native hardware-target C++ first; historical Web Audio/Python code are *reference prototypes*, not production DSP.
- No claims of real circuit-modeling quality without independent tests, audio auditions and actual hardware profiling.
- Reuse legally suitable DSP code where useful; respect license provenance; use SPICE as an offline reference where helpful.
- Scope freeze for pilot; future biometrics, creature-type grammar, battles, full 48 effect families, and aesthetic web configurator are not first-release commitments.

## What exists in this repo right now
- `include/parable_core.h`: in-memory stage/patch validation and a trivial audio core.
- `tests/test_core.cpp`: limited host smoke tests.
- `firmware/daisy_pass_through.cpp`: **uncompiled/unflashed** demonstration adapter.
- `CMakeLists.txt`: host test build; `.github/workflows/host-tests.yml`: new host CI configuration.
- No real circuit-modeled drive, no actual Daisy hardware proof, no serialization or persistent storage, and no phone editor. Do not conflate a passing smoke test with product readiness.

## First Codex task (intentionally narrow)
1. Inspect every tracked file, summarize the implemented features and most important limitations.
2. Run `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`, `cmake --build build`, and `ctest --test-dir build --output-on-failure`. Report actual output.
3. Review `AGENTS.md` and `PRODUCT_CONSTITUTION.md` for contradictions; do not silently change them.
4. Draft `docs/ADR-001-patch-contract.md` with two viable approaches for a versioned, validated patch schema and atomic runtime patch activation. Include real-time safety, four-slot persistence and JSON/binary interoperability. Mark it **PROPOSED**; do not implement either alternative until founder review.
5. Report all changed files, test results, missing evidence, and one suggested next small vertical slice. Open a PR for review; do not merge it.

## Quality gates and process
- Keep changes small, focused, buildable, reviewed. No unapproved redesign or new correspondence rule.
- Known missing proof: Daisy flashing, analog I/O, CPU, noise/latency, patch persistence, audio model quality, and user testing.
- Keep an explicit distinction between Specified / Implemented / Verified / Auditioned / Hardware-proven / Release-ready.
