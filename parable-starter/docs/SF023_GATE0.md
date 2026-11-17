# SF-023 — Wallace experiment, audit before implementation

Base main767a3c402b972fc20df8aea0729adf794f4b7a38. Read project AGENTS,
Constitution, README/status, Soundforge recipe, existing Muff/Rotary/Tape reports
and actual APIs, SF022 report and exact hall implementation. No root AGENTS exists.
Baseline before edits: core/audio-safety/shared Soundforge tests **5/5**,1.00s;
Release configure/build and exact logs retained. Scope explicitly excludes
redundant algorithm-level/SPICE campaigns; existing models will be hash-protected.

Exactly four processors: main Muff4x, Rotary, Tape; exact selected Hall from
PR77 commit21905d5, without merging or modifying it. No new effects, patch/Engine,
correspondence, firmware, living source, UI or general graph framework.

Lifecycle: stopped prepare, caller-owned tape medium, fixed64-frame Hall blocks,
48k mono. Muff native units use calibrated0.2 software input peak and jack/9 output;
no physical pickup voltage assertion. Parallel effects use their actual wet-only
mix settings; direct fuzz counted exactly once. Nominal48-frame Muff FIR delay
must also be applied to optional clean blend, which defaults to zero.

The established first20s take03 lacks its final long decay. Exact full original
SHA84980b54... is available; select source9.28–29.28s from the SAME performance.
This contains source10.62–10.86/13.16–13.54/18.08–18.34/20.54–20.96/23.50–24.18
quiet gaps and its final natural decay. No artificial gating/time stretching or
new recording rights needed. Add9s tail; common29s mono fits existing WAV utility.

FFmpeg7.1.5 ebur128 is available for host-only integrated BS1770 loudness/true-peak
analysis. Matching uses constant gain, not loudnorm/compression/limiting. No FFmpeg
code is linked/imported into DSP. No claim that LUFS forces equal subjective tone.

ADR023 fixes three hardwired routings and initial fair gains before code. Founder
listening answers the musical question; measurements cannot approve superiority.
