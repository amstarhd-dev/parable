# ADR-022 — complete borrowed hall, one scattering fault

Status: experimental selection for this assignment; musical/hardware approval open.

Select Chris Johnson's MIT **Airwindows kCathedral** at
`143a86df3642881e0be4bf827815050721063b41` after running it and complete Verbity on
the same guitar. kCathedral has explicit early reflection networks plus a mature
five-stage five-path late network per channel. It retains meaningful stereo
output on mono excitation; Verbity's symmetric network collapses to mono with
identical left/right input. Original default measured mono T20 extrapolation is
4.89 s at48k. Its fixed ~475 kB state fits available SDRAM; actual target timing
is still required. Verbity is much cheaper on the host but lacks that specific
early/stereo architecture. Neither measurement proves professional quality.

Preserve the entire original float-I/O processor's double arithmetic, delays,
early network, filtering, interpolation, nonlinear bass treatment and gain law.
Remove only VST metadata/host machinery from native DSP. Keep an executable
unaltered upstream reference, exact hashes and a reproducible mechanical adapter.
Expose a bounded decay-loss scaling of the existing loop; default reproduces
original settings. Add no new reverb tank, shimmer or chorus algorithm.

One fault hypothesis: a concave/coherent rear reflecting plane redistributes
less energy diffusely. At the final existing scattering plane replace normalized
Householder H with `(1-f)H + fI`, where f=0 healthy and a fixed high f is damaged.
Apply to each complete five-path vector, preserving the original cross-channel
connection map. Both endpoints are orthogonal; the convex blend is contractive.
This changes phase/path coherence without adding loop gain, a separate oscillator
or a threshold gate. Existing delay paths produce the resonances and existing
losses extinguish them after muting. This is a physically motivated boundary
reduction, not a measured architectural/acoustic model. Ordinary acoustic focusing
is substantially linear: weak/strong differ in absolute excitation, not an invented
input-dependent room geometry. Listen and measure whether that is sufficient for
the requested "largely normal" weak response; report failure if it is not.

Native storage is fixed and caller-owned; stopped prepare/reset only, no hidden
heap or reprepare double allocation. Controls are same-owner, not concurrent.
Mono reads the stereo sum; publish both mono and stereo evidence. Do not integrate
Engine, canonical patches, living modulation or target production firmware.

Tradeoffs: source has intentional near-zero noise/dither, fixed-size timing and
sample-rate-dependent interpolation, and expensive double libm operations. Keep
those initially rather than claiming an optimized port is the proven original.
Stop on memory infeasibility; do not infer deadline safety from host speed/linking.

Observed outcome: healthy/adopted3rate float samples agree exactly; fixed474704B
Hall and full BOOT_NONE95348B flash-load audio image linked in unchanged regions.
Actual host medians0.316/0.299µs/frame, no target timing. Final faultf0.98 has
changed late spectrum/chord bloom and extinguishes in sustained tests. It does
not become relatively identical to healthy under weak excitation; that musical
requirement is unresolved. Reset is stopped-only with an external audition fade.
Professional quality/physical hall fidelity and Seed3 admission remain open.
The original96k input subsampling drops a phase0-only one-sample impulse; the
corrected response probe excites both phases. No antialias retrofit is hidden.
See SF022_REPORT for exact evidence, rejected estimate, and next smallest test.
