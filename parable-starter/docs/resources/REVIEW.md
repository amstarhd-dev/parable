# Independent review of draft PR81 and82

Both remain unmerged. Reviewed heads: PR81 `11404fd192921d4e3a93c944856eacc888bb4b09`,
PR82 `067da922edc0bde7c2d3187dfcaf5fe3993af097`. This is a source/evidence review,
not a musical approval or authorization to merge either proposal.

## PR81 — retain experimental status

1. **Shorter FIR has no acceptance gate in the existing audit.**
   `tools/rat_candidate_audit.py` prints97-versus193 error but only rejects the
   float-clipping comparison above0.05%. Its eight probes do not establish
   anti-alias equivalence, transient quality or musical acceptance of the97-tap
   replacement. Keep193 as reference. FIR latency changes48→24 base frames;
   align full recordings and dry branches before judging their differences.
2. **Physical cost evidence has advanced beyond the PR description.** The founder
   reports370915,270757 and235475 maximum callback cycles for the193/double,
   97/double and97/FP32 versions. Those results are credible physical observations
   at480MHz/48k/48 frames, unconnected input. Exact binary/report binding was not
   supplied to this audit, and SDK/ISR overhead is incomplete. Do not relabel these
   as isolated-RAT WCET or full-chain admission. Preserve them as reported evidence.
3. **Correct the explanation of double precision.** The pinned SDK selects
   `-mfpu=fpv5-d16`. Its compiled RAT path contains native `.f64` instructions,
   with no software-double arithmetic helpers in the inspected callback graph.
   The FP32 experiment can improve hardware throughput without eliminating
   software emulation. The measured extra savings after97 taps are about13.03%
   in the reported composite; the total reduction is about36.52%.
4. **The1x image is a CPU lower bound.** It removes anti-aliasing and is explicitly
   not a fidelity candidate. The new catalog/harness includes it without promoting it.

Default/reference algorithms remain preserved. Broader fault/low-level/high-note,
latency, alias and actual guitar/amp comparisons remain prerequisites for adoption.
No new algorithmic change or automatic merge is justified by this review alone.

## PR82 — useful starting ledger, incomplete admission machinery

1. Its21-voice list is valuable but mixes historical host observations and mutable
   branch names. It lacks a common measured protocol, exact build identities and
   per-case counters. This audit supplies those; it also separates the generic
   prototype, alternate CMOS branch, germanium variants and Engine utilities.
2. `check_daisy_admission.py` identifies RAT by substring `RAT_COMPOSITE` and uses
   an effect-level selector that cannot bind193,97 andFP32 variants to distinct
   binaries. A stale or different variant can be attributed to the wrong source.
   The new importer requires an exact versioned artifact ID/source-set match,
   rejects mixed/partial/duplicate-conflicting captures, and preserves execution
   context, clock, block, cache/FPU settings and observation scope.
3. The ledger's “soft-double” shorthand is not established for this target. Native
   hardware double instructions and software transcendental functions are separate
   optimization questions. The new ARM audit reports reachable dependencies.
4. Memory needs lifetime/context. Core-only Cloud size is67011904B on ARM
   (`coreBytes()` returns0x03fe8540); this fits64MiB by96960B **before other state**.
   Observed host wrapper/heap requests are67028776B, and stopped reprepare peaks
   at134070296B. Two ARM cores alone need134023808B. Unchanged overlapping
   preparation cannot fit64MiB. This is not proof a single core could never run.
5.70% whole-graph body allocation is a provisional engineering reserve, not a
   certified hardware limit.20% drive/12% reverb envelopes are planning hypotheses,
   not automatic musical-quality tradeoffs. Do not replace a whole-graph capture
   with summed isolated maxima or a host-to-M7 conversion.

The new PR builds on the purpose of both drafts without merging them or altering
production DSP. Source-preserving tooling and explicit holds should precede any
shared FIR, numerical or memory-layout optimization.
