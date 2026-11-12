# SF-005 Gate 0 — Gemini sound first

Base main `8f943681ad933f7d8b65d1d66535aa811c75c48e` (merged #33).
Read issue #41/comments, project AGENTS/Constitution/README/status, ADR-004,
CMOS report/model/device equations/lab/SPICE references, Soundforge methodology,
PAR-008 Gate 0, patch-v1 codec/bridge, Engine and current integration/activation
tests. Root AGENTS.md is absent; project rules are under `parable-starter/`.
Parable Lexicon v1.0 is not tracked here; do not invent its missing rules/values.
The issue itself authorizes the inverter bias/feedback fault family and conditional
additive integration. No wearable/biometric or PAR-012 changes are in this branch.

Clean pre-edit host baseline: **23/23 Debug (116.24 s), 23/23 Release (73.72 s)**,
GNU 14.2.0/CMake 3.31.6. Preserve old render/fixture hashes before DSP edits.

## Electrical audit and candidate

Reuse SF-002's MIT-adapted CD4049UBE complementary MOS currents/Jacobian and fixed
two-output trapezoidal/Newton solver; source pin/notice and fit limitations remain.
Whole circuit 4x FIR, <=16 Newton loops per internal phase, fixed state; no callback
allocation/locks/IO. Coefficients/DC initialization remain stopped-only.

`feedback_low` (Rf2 1 Mohm -> 100 kohm) cannot shift DC bias in this reduced
circuit: capacitors open at DC and zero gate current forces gate=output. It is an
AC gain/bandwidth control, not automatically the requested bias malfunction.

Test ONE physical addition: leakage from second gate to ground, normally open,
candidate **Rb=330 kohm**. Represent it as conductance (0 S=open), never a fake
waveform overlay. Select the final single value from electrical/zero-fallback
listening evidence; document any candidate rejection. No severity/fault taxonomy.
At DC g2=o2*Gf/(Gf+Gb), and inverter current+(o2-g2)/Rf=0. Gate/output bias must
both be solved; reset capacitor histories to that actual equilibrium. At audio,
the same shunt changes the gate KCL denominator/Jacobian. This remains a dedicated
2x2 solve, not a general solver. Independent ngspice stamps/residuals must validate
the new branch; no gating can be credited to numerical fallback.

First deliver same-input healthy/fault WAVs and level-matched receipts. Retain
original SF-002 healthy/coupling/feedback lab outputs unchanged. Source permitted
clean DI if available (with explicit recording license/provenance); otherwise
label synthetic stimuli and report the real-guitar acceptance gap. Human musical
approval belongs to the founder. No physical-unit calibration is implied by PCM.

## Narrow integration/resource gate

SF-002 state is ~2.1 KiB versus RAT ~10.7 KiB; use ONE fixed tagged union in each
Engine, not a second optional state alongside RAT. Preserve all eight utility
stages and the four PAR-011 bundles. At most one nonlinear declaration total,
including disabled instances. Existing two-RAT rejection remains unchanged.
Native stage controls can share their existing 12-byte space. Measure complete
Engine/typed-document/bundle/Runtime layouts; do not silently grow the pool.

If safe, register exact type `GeminiCmos`, revision 1, within the existing additive
v1 registry pattern. Required drive/volume normalized quantities plus a strict
boolean `biasLeak` quantity select healthy or the one fixed damaged component.
No default, interpolation, astrology rule, UI state or migration. Old canonical
bytes/parameters/types/routing remain unchanged; older readers reject this type.
Processor-specific validation is sufficient; no general plug-in/parameter system.
Configure inactive state on control; one union alternative/history is audio-owned.
Mixed RAT/CMOS graphs reject rather than allocate extra state. Stopped load failure
must preserve configuration/history; prove actual Runtime and four-slot behavior.

Recheck native 32–96 kHz, gain/order/disabled/bad-input semantics, callback guards,
full Debug/Release and affected sanitizers, legacy golden bytes, pinned ARM objects,
layouts, actual link footprint and bounded static work/host timing. Existing pins,
linker memory maps and hardware behavior claims stay unchanged. If this cannot fit
without a larger Engine/schema/lifecycle decision, stop at the completed standalone
audio pair and report the blocker. No such blocker is established by this audit.
