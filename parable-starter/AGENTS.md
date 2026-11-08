# Parable — Coding Agent Rules (proposed)

You are working on a real-time embedded guitar-effects product targeting Daisy Seed3 and a portable native C++ core. Treat browser v3.3 and the old Python renderer as historical references, not shipping DSP sources. Read PRODUCT_CONSTITUTION.md before changing scope.

## Nonnegotiable
1. Never claim circuit-model fidelity, musical quality, or real-hardware compatibility without tests that actually prove those points. `nonzero audio` is plumbing evidence, not pedal-grade evidence.
2. Never add astrology rules, revise correspondence semantics, or choose a new effect topology without a written decision record for founder approval.
3. Never use UI state as an implicit source for a compiled patch. Validated versioned patches are the single playback source of truth.
4. No allocations, I/O, network actions, exceptions, unbounded work, or blocking locks in the real-time audio callback.
5. Every patch edit is validated before atomic activation; malformed patches cannot disconnect the live output or produce unbounded levels.
6. Work in small changes; preserve clean build, tests, rollback route, and prior project history. Don't refactor unrelated systems.
7. Keep source, headers, tests, samples, docs in the same Git repository. Do not deliver unintegrated ZIP rewrites as final proof of progress.
8. Code licensing matters. Record upstream provenance and license for borrowed algorithms; no copy-paste of commercial closed-source models.

## Every assignment report
- What files changed and why.
- How to reproduce build and tests on a clean checkout.
- Which acceptance criteria passed and which failed.
- CPU/memory and real-hardware evidence, or explicit 'not measured'.
- Musical fidelity evidence, or explicit 'not auditioned'.
- Risks, limitations and the next smallest test.

## First assignment
Read README and tests. Build the host C++ core, run tests, and report actual results before changing code. Then draft ADR-001 (patch schema / versioning / serialization / authoritative runtime snapshot) with exactly two options and tradeoffs; do not implement until architecture is approved. Identify a smallest independently testable follow-up change.
