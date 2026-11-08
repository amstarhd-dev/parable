# PARABLE — Product Constitution & Founder Operating System
Version 1.0 — 2026-11-06
Status: proposed operating baseline for founder approval. This file records decisions and explicit hypotheses; it is not a claim that features are implemented.

## North star
Parable is a playable correspondence instrument: it translates structured symbolic relationships into audibly meaningful audio-processing graphs. Its flagship grammar is traditional Western astrology. Manual Workshop mode lets musicians inspect, rearrange and save the same processors. **Correspondence is the distinctive invention; Workshop makes the instrument accessible and commercially useful.** Neither mode is allowed to require the other during a performance.

## Product promise
One guitar pedal, four locally stored and instantly recallable independent patches, Register, Volume and bypass. A browser-based phone editor lets players build a virtual pedalboard and generate/edit patches from a birth chart. After saving, the pedal operates without phone/computer/network. Its core processors must stand up to ordinary guitar use without any appeal to astrology.

## Audience hypotheses (not validated)
Primary: curious developing electric guitarists who want an easier way to explore effects. Secondary: creative/experimental players and deep gear enthusiasts. Astrology may increase discovery and cross-audience interest, but hardware demand from nonguitarists is unproven. Research buyer behavior, not demographic stereotypes.

## Two composers, one runtime
- Chart Composer: chart facts -> traditional rules -> auditable correspondence trace -> validated patch graph.
- Workshop Composer: visual blocks and connections -> validated patch graph.
- Runtime: portable real-time C++ DSP engine -> target-specific hardware adapter.
- A patch is an immutable, versioned snapshot with explicit processor IDs, parameter units/ranges, routing, resource budget, expression mappings, and deterministic preset serialization.
- Neither UI state nor compiler globals are permitted to silently override a loaded patch. Round-trip save/load is a required test.
- Audio callback: no unbounded allocations, filesystem/network calls, or unsafe graph edits. Heavy compilation and graph changes occur off the audio thread.

## April 1, 2027 target
**A credible small pilot, NOT a promise of broad retail launch.** Pilot definition: real guitar in/out, acceptable noise/latency, reliable footswitch and four persistent preset slots, working Register & Volume, simple usable phone Workshop, and real chart-to-sound workflow on the same graph. Processor count intentionally small and subject to hardware benchmarks. Circuit fidelity honestly documented. No claims of 48 finished circuits.

## MVP Must / Later / Not Now
### Must for pilot
- Genuine reproducible C++ builds on host and Daisy, guitar input/output, safe gain/feedback.
- Four physically selectable saved presets surviving power loss; independent Register, Volume and bypass; no phone required to play.
- At least 2-3 genuinely distinct, well-tuned native drive architectures, one modulation, delay, compression and reverb at acceptable quality. Exact counts are a release tradeoff, not marketing commitments.
- One small but astrologically meaningful traditional chart compiler that **audibly changes** at least processor selection, parameter behavior and routing. Publish any deferred rules plainly; preserve complete canonical grammar as long-term spec.
- Phone Workshop: add, remove, reorder, tune, save and recall a simple serial chain; usable on both iOS and Android or report the platform limitation before making any purchasing claim.
- Smoke-tested switching/level behavior, measured CPU/memory, audio comparison with real guitar, build reproducibility, preliminary BOM and supply backup plan.
### Later (extension points only now)
- 12 complete circuit-grade drives, the entire Moon 12-mode set, comprehensive aspects/11th/12th transformations, parallel multi-graph editing, variable analog pickup loading, biometric compiler, non-infringing creature/type compiler, extensive scene/morph engine, broad community library.
### Not now
- Battle mechanics, e-commerce customization configurator, elaborate storytelling website, custom STM32/codec module clone, neural model training, general-purpose amp/cab modeling suite, 48 polished effects, multi-platform app store installs, subscription ecosystem.

## Decision rights
Founder: artistic correspondence semantics, musical acceptance, customer promise, appearance, pricing and scope tradeoffs.
Technical reviewer (AI-assisted): architecture recommendations, security/reliability risk warnings, specs, audit and independent tests; must make uncertainty explicit. Cannot approve its own unverifiable quality claims.
Coding agent: implement one ticket at a time on a short-lived branch; no silent feature additions or unrelated rewrites.
Actual hardware and listening tests: final authority on real-time cost and sound. Customer tests: final authority on ease of use and market fit.

## How work moves
Idea -> decision record (ADR) -> small ticket with acceptance tests -> implementation branch -> automated checks -> independent review -> merge -> hardware/audition evidence -> status update.
Never describe a feature as finished when it is only described in documentation or wired to synthetic nonzero-audio tests.

## Definition of done
Code can be built from a clean checkout; tests actually exercise named behavior; patch round-trip and error paths verified; bounded CPU/memory; change is documented; audio artifact or live guitar audition exists when sound quality claimed; no compiler/renderer/UI drift. If any criterion is absent, state the precise completion stage.

## Core quality metrics / proof gates
- Measured real-device round-trip or input-to-output latency, worst-case callback CPU load, heap/stack/memory use, silence/noise floor, peak output, clipping/feedback containment, preset switching transient, power-cycle persistence.
- Model quality: input-level and guitar-volume sensitivity, alias/DC/stability tests, loudness-matched comparisons, documentation of analog-circuit approximation.
- Usability: a new guitarist can build/reorder/save a board with little instruction; count steps, failures and required hints.
- Correspondence integrity: reproducible chart fixtures; nontrivial change to audio from changing a relevant chart feature; trace each audible decision back to a stated rule.
- Commercial viability: BOM, assembly/test minutes, yield, returns/warranty reserve, price-specific buying signals.

## Scope control
The April pilot date is a constraint. New ideas enter an **After Pilot** parking lot by default. Bringing one into the pilot requires founder-approved replacement of equal or greater workload, with explicit schedule/risk tradeoff. A ticket cannot say “build 12 circuit-grade distortions”; it must specify one audible outcome and tests. No big rewrites without a rollback path.

## Weekly operating cadence (small team)
Monday: choose a single primary engineering outcome and at most 2 supporting tasks. Wednesday: demonstrate running software or hardware, not slides. Friday: review tested evidence, risks, cost, what failed, and next week's priority. Once a month: obtain at least a few external buyer/user reactions; record observations separately from suggestions.

## Current gates (from November 6 recovery audit)
- 2026-11-20: architecture/preset schema freeze and patch-state consistency.
- 2026-12-11: first genuinely different native drive models and performance proof.
- 2027-01-08: real standalone guitar -> Daisy -> amp prototype, bypass/register/volume.
- 2027-01-29: four persistent hardware presets.
- 2027-02-19: phone Workshop save/edit on hardware.
- 2027-03-05: feature and sound-quality lock for pilot.
- 2027-03-19: 5–10 guitarist field-test gate.
- 2027-04-01: explicit go/no-go decision for small pilot.
Dates are targets, not completion claims. If real hardware falls behind, cut scope or classify release as engineering beta, not pretend milestones passed.

## Immediate priority
Before another sound or UI feature: put Parable Core Brick 0.1 into a private Git repository; verify a clean host build/tests; write ADR-001 for one patch schema and single authoritative patch state; make compiled patches valid/invalid deterministically; build four-slot persistent serialization tests and a hardware-compatible effect-instance/graph budget. Separately acquire hardware for pass-through work and verify toolchain. Native Daisy audio pass-through must be demonstrated early, not just promised.

## Reading list (practical sources)
- DORA small batches/continuous integration: https://dora.dev/capabilities/continuous-integration/
- GitHub branch checks and protected main: https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches
- YC startup library / MVP and user interviews: https://www.ycombinator.com/library
