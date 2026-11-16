# SF-020 Gate 0 — before DSP edits

2026-11-16; main767a3c402b972fc20df8aea0729adf794f4b7a38. Issue73 is
SF-020 (not a new correspondence decision). AGENTS, Constitution, README,
implementation status and Soundforge recipe inspected. PR48 spring passive
contact and PR72 plate/clearance decisions inspected through their actual PR
reports for methodology only. Neither branch imported or modified.

Baseline: GNU14.2/CMake3.31.6 Release shared utilities + positive/negative
registry3/3 pass,1.00s. Exact log accompanies report. Legacy effect suites unchanged;
issue explicitly requests focused gates. Checkout omits old listening samples via
local sparse checkout to fit workspace; no tracked file removed. Completed host
build intermediates reclaimed; older binaries reversibly gzip archived.

## Borrow/adapt comparison and decision

Signalsmith Basics MIT reverb pinned369e906e03760ec219d07b06fa06dc4fd099f377:
inspect reverb.h and actual LICENSE.txt. Efficient Householder FDN with damping,
modulation and diffusion is credible prior art. Whole STFX/framework/delay/mix
integration is unnecessary for a mono two-room experiment. Adapt its mixed-feedback
Householder pattern, retain notice; implement two fixed four-line groups and one
explicit bidirectional coupling edge. No framework/dependencies imported.
The older Signalsmith example2798498 has no explicit license file in its root;
audit-only, no import. JUCE/commercial algorithms unnecessary.

Own reduction: room A four short propagation loops, B four longer loops with
separate reflection attenuation/low-pass absorption. Couple one port from each
through [[1-c,c],[c,1-c]],0<=c<=1. Fault changes ONLY B-return-to-A coefficient
+c to -c, representing a miswired electronic room-return pickup. Both matrices
have spectral norm<=1 (triangle inequality); losses strictly contract. No physical
room geometry, calibrated aperture or electrical hardware reconstruction claimed.
The aperture is a passive reduced mixing connection, not exact lossless acoustic
scattering. Input A only; B silence when c=0 is mandatory. Primary mono sum retains
feedback-interference consequences; no added random/crash signal, modulation,
nonlinearity or hidden normalization. First actual guitar WAV and impulse proof
precede elaborate tests. Founder musical character decision remains pending.

Prototype observation: aperture.35 produced only~8% raw guitar difference.
Selected aperture.85 after~37% raw difference with all other parameters identical.
The permitted0..1 range retains the same contraction proof; no second fault.
