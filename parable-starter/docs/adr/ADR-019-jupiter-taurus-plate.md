# ADR-019 — Dattorro-style diffuse plate with reduced unilateral mount

**Status:** experimental implementation, human/physical admission withheld.

Taurus/Jupiter specifies a plate reverb and a damaged suspension/damping assembly. We chose a C++17 figure-eight Dattorro 1997 network because a working MIT-licensed design exists and an expensive 2D flexural plate solver is not justified for the first guitar audition. Studied [johnhw MIT C](https://github.com/johnhw/dattoro_reverb) and [Mike Jarmy MIT C++](https://github.com/mjarmy/dsp-lib); reviewed GPL Plateau/Hothouse and AGPL ModalDish without importing copyleft code. See source hashes and notices.

**Decision:** Retain identifiable two-tank delay/diffusion/feedback topology for the dense healthy tail. To try the **physical fault hypothesis**, attach a single damped transverse mount mode to the wet-body signal. Reflect outward velocity when the modeled displacement reaches the contact gap, and return the bounded impact impulse into the same reverb's input diffuser on the next sample. The contact gap alone differs between healthy/fault. This is a reduced *hybrid*, not a steel membrane calculation, measured mount geometry, spring body, or validated EMT 140 reconstruction.

**Known limitations:** The Dattorro tail does not offer physical plate displacement. Modal displacement/clearance use dimensionless coordinates. The impact energy loss is local; the composite with feedback injection is only empirically bounded in current tests. Max-CPU and four-bundle SRAM are unproven on Daisy. One sample audio output is mono sum of synthetic stereo taps, and its phase behavior has not been musician-reviewed. No runtime integration or canonical patch change.

**Admission:** Keep this PR unmerged until founder audition and missing sanitizer, target object and spectral work. It is an executable playable-first experiment, not a claim of production readiness. See [SF019_REPORT](../SF019_REPORT.md).
