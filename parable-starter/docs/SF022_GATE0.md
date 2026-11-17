# SF-022 — audit before implementation

Assignment: [complete supplied specification](SF022_ASSIGNMENT.md). Base main
`767a3c402b972fc20df8aea0729adf794f4b7a38`; Cancer PR #76 is not imported or modified.
AGENTS, Constitution, README, current status and Soundforge recipe were read.
Existing native Engine/patch/storage/firmware remain outside this increment.

Before repository code changes, clean Release configure/build and all **40/40**
existing tests passed in **142.85 s** (GNU14.2, CMake3.31.6). Exact logs are in
`data/sf022/sf022-baseline-*.log`. The JSON source override uses the existing
pinned3.11.3 source cache, not another effect implementation.

Research first: original complete Airwindows kCathedral and Verbity processors
were compiled and rendered the same prepared take03 before selection. Original
MIT headers, constructor, float and double processing are retained verbatim.
The laboratory's own minimal VST type/host shim is not the Steinberg SDK.
Measured sizeof: kCathedral474752 B / Verbity1007232 B on this host, with small
plugin metadata allocations separately measured. Both are far below64 MiB.
Cloud's oversized storage is not reused. Deployment still requires real ARM
placement/call/stack evidence and physical timing; compilation is not admission.

Freeverb3 Progenitor/ZRev2 and MVerb were rejected for import under the current
MIT/BSD2-only repository policy: their source is GPL. Signalsmith Stretch is a
pitch/time processor, not a hall. Signalsmith Basics is a credible reverb but
its complete STFX wrapper includes a separate Boost license and M7 atomic64
integration work; no license policy expansion or Cancer changes are needed here.
Dattorro C alternatives were inspected: el-visio is fixed-delay-rate double
plate code; johnhw's left4453 delay is read without a corresponding write in
compute_reverb. Neither is silently repaired/imported as our healthy hall.

The selected candidate decision and fault hypothesis are recorded in ADR022
before native adaptation. Musical quality is pending listening, not established
by a decay measurement. Current main has no reverb/"generic ambience" processor
to A/B; an unavailable historical baseline will not be fabricated.
