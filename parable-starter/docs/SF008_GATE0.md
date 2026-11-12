# SF-008 Gate0 — before native edits

2026-11-12. Main **a17f64fe222dc5c9867c693fb3b0e4e69c71f465** includes approved
#46; #42 remains open/unmerged. No other branch is merged/cherry-picked/resolved.
Read AGENTS/Constitution/README/status, Soundforge, ADR-001/PAR008/PAR011,
ADR-006/007, canonical/native code and ARM reports. Frozen Lexicon and root
AGENTS.md are not tracked; issue#47 authorizes this sound study, not activation.

Fresh GNU14.2 / CMake3.31.6 baseline **30/30 Debug (131.63 s) / Release (87.79 s)**:

```sh
cmake -S parable-starter -B /workspace/work/sf008-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf008-Debug -j2
ctest --test-dir /workspace/work/sf008-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf008-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf008-Release -j2
ctest --test-dir /workspace/work/sf008-Release --output-on-failure --no-tests=error -j2
```

## Research / selected gap

MIT Rings `rings/dsp/resonator.cc`, eurorack
**f4cfa7697095a28d292bff758beb51deb49b5a86**: inspected file-level permission,
positive stiffness/stretch-mode recurrence and spatial modal weighting. Narrowly
adapt this modal foundation; use explicit mass-normalized energy coordinates
instead of its opaque filter states, so contact can be physically accounted.
Two192-total dispersed spring-path surrogates, not a generic room/hall or a
calibrated helical-spring reconstruction. Original gap: energy-bearing modal
state and one support contact, not an available imported crash implementation.

MIT audiojs/reverb **29597c9f7038facbdda6cdf3ef361f72cf35482f** spring24-allpass
loop evaluated. Its license explicitly retains MIT beneath a non-overriding
preamble. No code imported: wave-variable contact energy/displacement needs a
larger derivation, and its source's "calibrated" label is not independent proof.
Jacob McQuillan spring-tank model **6b16988986a59aabaece645981a4c0fbe01dd6a7**
tree has no license file; no MATLAB/listing/recording imported. Bilbao/Parker
literature distinguishes real helical longitudinal/transverse coupling and
dispersion from this reduced modal surrogate. Source pins/notices/gaps retained.

## Bounded scope / admission

Fixed **2 paths x96 modes**, two substeps/audio frame, prepared coefficients;
32–96 kHz mono. Exact lossless modal rotations plus passive per-mode amplitude
attenuation, force kicks and projected contact. 193-tap Blackman output decimator
uses existing owned FIR idioms, without editing protected kernels. No simulator
framework, noise source, extra crash track or unbounded event solver.

Hard ceiling **24576 B complete processor /8192 B ARM object text**, no heap or
external SDRAM. Stored modal energy <=**2 microjoules** via an explicit input-port
energy budget, never hidden output normalization. Healthy clearance5 mm;
one damaged path's support clearance proposed20 micrometres, e=.55 restitution;
selected fixed value is confirmed with weak/strong and same-guitar probes.
All other coefficients/coupling/energy budget remain equal. Position projection
and restitution losses are reported separately; numerical projection must not
masquerade as physical collision energy. At most one contact decision/substep.

Minimum amplitude loss3/s gives a conservative unforced decay bound; **6 s**
maximum stored tail after last nonzero input, with a mathematically sub-audible
passive clear. Return is bounded using Cauchy–Schwarz energy/pickup norm and FIR
L1 norm, not a cosmetic limiter. Record overload port limiting separately.

**Standalone host specimen only.** Current Engine11040 B /bundle11288 /Runtime
45224 B and optional RAT capacity10752 B are authoritative. This proposed complete
state exceeds that capacity; do not expand pools or adopt unmerged#42's union.
The existing seven actual firmware images/pins/linkers remain unchanged, first-
boot RAT109784 B internal-flash span; none demonstrates new spring CPU/RAM fit.
Compile/audit the standalone M7 object and rerun old links, no target run claim.

No measured target CPU budget exists. Future admission goal <=2000 M7 cycles/
sample is **not** a measured result; the full modal specimen may exceed it. If
so, independently audition a smaller modal truncation/optimized structure in a
separate ticket, keeping contact/energy gates and actual sound comparisons.
No hardware-dependent memory, live switching, control architecture or astrology.
