# SF-018 — Capricorn: original DS-1 family and C4-open

Experimental standalone reduced guitar DSP, not an authenticated1978 specimen.
[Nine listening WAVs](../samples/capricorn/README.md), [ADR-018](adr/ADR-018-capricorn-c4-open.md),
[pre-code audit](SF018_GATE0.md), [sources/rights](../third_party/capricorn/provenance.json).
First native WAV **10min43.028s** after starting (4x prototype); final8x files
identify producing code and hashes. Active labor was not tracked. No human audition
or founder approval is claimed.

Original Q2 bias/feedback and nonlinear transistor bite precede a bounded reduced
TA7136, antiparallel silicon hard clipping and loaded parallel Tone. Original
C1/C5/C8 values are retained; no post1994 dual-opamp BOM. One fault only: C4
250pF across470k R7 is absent. DC base/collector/emitter remain exactly identical
in native healthy/open; collector≈4.2978V. Independent Q2 gain increases≈4dB at6kHz
and≈5.5dB at12kHz. No invented EQ/extra fault. On the same take03, raw level rises
0.605dB low Distortion and0.064dB high. After RMS matching,6–12kHz energy rises
3.91/2.92dB respectively. This supports the predicted brighter-frequency change,
not a claim that it sounds better or is perceptually a convincing DS-1.

**Credibility:** ngspice44.2 independent reduced Q2 DC/static/AC, loaded Tone and
diode-companion checks: DC error1.05µV, static curve1.32µV at nine sampled points,
diodes0.23µV, Q2 magnitude0.154%, Tone0.080%. Observed clip peak0.606V, amplifier
bounded3V. High-note7.969kHz spur1x/4x/8x: healthy−15.3/−42.8/−56.2dBr,
open−15.0/−39.2/−50.3dBr. The measured11–13dB improvement selects8x; residual
aliasing remains. These sparse checks validate the declared reduction, not devices.

**Cost:** full native host1.135–1.250µs/frame, GNU14.2.0 Release,432000-frame repeat
per combination after builds finished. First4x prototype0.618–0.647µs/frame;
8x costs approximately twice as much. Pinned M7 object text11536B/data0/BSS0;
instance8832B. Fixed eight internal steps,386 FIR products,≤376 companion MACs,
bounded curve searches/ADAA/RC arithmetic; no callback solver/exponential, heap,
I/O, lock or double math. Static callback frame chain168B before caller/interrupts;
stopped prepare own frame11312B plus callees. **No Daisy cycles, firmware link,
boot, hardware audio or deadline evidence.**

**Checks:** Debug4/4, Release4/4 (new circuit/CLI and changed registry), ASan+UBSan2/2;
all Release targets build. Existing DSP/headers/firmware/fixtures remain unchanged.
Old full circuit/listening suites were deliberately not run locally, per issue68;
existing global CI remains enabled. [Exact commands/raw evidence](data/sf018/README.md).
Changed files: isolated native core and host lab; compact tests/SPICE fixtures and
ARM specialization; nine credited WAVs/receipts; ADR/status/registry and corresponding
CMake/CI steps. Inventory assertion12→13 only; no legacy physics thresholds changed.

**Limits/next:** Q2's nonlinear C4 dynamics/charge, TA7136 internal compensation,
headroom/bandwidth, actual transistor/diode spread and buffer loading are declared
approximations. Physical fidelity and perceived distinction from RAT/Muff/TS808
remain unproven. Next smallest gate: founder low/high matched A/B, then actual
M7 timing/reference-pedal evidence if retained. No Engine/patch/correspondence changes.
