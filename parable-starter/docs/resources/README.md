# Batch resource audit — first deliverable

**21 primary voices;35 native build identities;12 mixed-chain configurations.**
Production DSP, canonical patches, storage, activation, correspondence and normal
firmware are unchanged. PR81 and82 remain unmerged. The deliverable is the audit,
repeatable benchmark/admission evidence and hardware handoff, not a claim that
all chart combinations currently run on Seed3.

- [Priority ranking](RANKING.md).
- [Complete measured matrix](MATRIX.md) and [machine matrix](resource-matrix.json).
- [Review of PR81/82](REVIEW.md), [pre-edit audit](GATE0.md).
- [Host raw ledger](host-ledger.json.gz), [ARM raw ledger](arm-ledger.json.gz),
  [all43 source snapshots / duplicate versions](source-inventory.json.gz).
- [Versioned benchmark images and evidence](resource-firmware-kit.zip),
  [artifact manifest](artifacts.json), [hardware measurement matrix](hardware-matrix.csv).
- [Source catalog](../../tools/resources/catalog.json) and
  [actual laboratory adapters/settings](../../tools/resources/adapters.h).

## What the audit establishes

Every active native processor source found across43 main/PR snapshots maps to the
catalog:98 distinct native file versions, no missing snapshots/unclassified kernels.
The21 voices include main and named open PRs; the additional builds distinguish
CMOS integration, the earlier generic drive, RAT experiments, five germanium
reference/approximation builds, and Bypass/Gain/Lowpass. Historical source versions
are inventoried rather than silently treated as additional effects. PR78/79 reuse
Hall and existing effects for host routing/cabinet comparisons. External unselected
Signalsmith research, offline IR convolution and absent browser/Python production
ports are not presented as admitted native effects. Unpublished SF026 is preserved
separately; it is not a GitHub production implementation.

All source is read from exact Git commits and compiled directly. No model equations,
coefficients, solver architecture, oversampling or earlier effect files are changed.
The benchmark tests nominal healthy, documented fault/parameter corners and high
controls, including secondary RAT/CMOS/germanium faults. Inputs include licensed
real guitar, plucks, silence, high notes, noise, edges, bursts/tails, DC, invalid
floats, finite extremes and denormals. This is a bounded screen, not exhaustive
parameter or tail-lifetime certification. Calibration/return units differ by model;
these are resource probes, not a loudness-matched musical comparison.

Host timing uses actual48-frame routines, separate compilation/measurement phases,
one selected CPU affinity, repeated fresh instances and output hashes. Median,
p95, maximum and mean callback wall time are retained. The timer includes host OS
interference; CPU frequency is not locked. **No host number is M7 utilization.**
Allocation tracking measures requested live and overlapping reprepare heap,
including the object; allocator overhead/global state is separate. Constructor and
prepare times include instrumentation overhead. Guards cover linked native allocation,
new/delete, selected locks and I/O; they do not instrument all libc internals.

ARM inspection uses pinned xPack14.2.1 and SDK-compatible M7/D16 flags. It records
instance bytes, native text/static sections, all compiler frames and reachable
callback math dependencies. Compiler frames are individual functions, including
stopped preparation, not measured stack high-water or an ISR-inclusive bound.
46/47 complete profiles compile under those target flags; the plate compile failure
is retained. All counters/commands/source fingerprints are retained. Zero reported failures is
not proof of convergence where a processor has no failure-counter API.

## Priority and common work

1. **Memory/lifetime hold: Cloud chamber.** ARM core alone67011904B, leaving96960B
   of64MiB before other state. Host live requested67028776B; reprepare peak134070296B.
   Two ARM cores alone exceed capacity. Investigate exact sizing of used delay
   storage and preparation lifetime, preserving the complete reference/audio.
   A single core's raw fit does not establish allocator placement, whole-chain fit
   or safe activation. No Cloud firmware image is offered as an admission claim.
2. **Target portability hold: plate.** Host measurement succeeds, but its current
   `catch(std::bad_alloc)` preparation code fails the pinned SDK
   `-fno-exceptions` compile. Preserve the source and resolve preparation/allocator
   policy in an isolated change; do not silently enable exceptions or claim target fit.
3. **Numerical hold: Blender; stress holds for CMOS variants.** Keep the recorded
   solver failures visible even when outputs are contained/finite. Repair/define
   acceptance before promoting a faster implementation. Existing Blender high-note
   issues remain. Do not speed it up by reducing convergence work or masking failures.
4. **Likely computational bottlenecks: iterative drive and magnetic families.**
   TS808, Klon, Muff, FZ1, germanium, Blender and tape saturation combine repeated
   nonlinear evaluation/solving and oversampling. Host ranking is in the matrix;
   actual target priority must be refined using the supplied physical benchmark kit.
5. **First shared optimization ticket: exact-order FIR ring traversal.** RAT,
   CMOS, germanium and several later drives repeat polyphase interpolate/decimate
   loops. A contiguous/mirrored history can remove wrap branches while preserving
   tap count, coefficient values and accumulation order. Measure complete processors,
   additional RAM and M7 performance; require output identity before adoption.
   This is a proposal, not a measured saving or implementation in this PR.
6. **Effect-specific work:** derivative-consistent exponential approximation and
   structured solver work require equation/derivative, convergence and independent
   reference checks. Existing germanium alternatives are measured, not auto-selected.
   RAT's FP32 clipping result does not authorize converting its preamp or every
   other circuit to float. Reverb `sin`/`pow` can be signal-dependent; don't hoist
   or approximate them as though they were constant parameters.64-frame algorithms
   require measured48/64 bridging and latency alignment. Buffer sizing belongs to
   each algorithm's proven index bounds/lifetime, not an arbitrary global cap.

No new computational optimization is adopted in this first deliverable. Prior-art
variants remain experiments, full-precision originals remain references. Subsequent
changes need isolated commits/PRs with errors, spectra/aliasing, transients, latency,
failure behavior, listening and full-processor before/after evidence. There is no
measured new Seed3 saving from this audit itself.

## Whole-chain implications

The12 configurations combine four drive/reverb pairs with the actual tape delay
and rotary: Scorpio/Rooms, Wavefolder/Tiled, RAT/Hall and Muff/Tiled. Each has serial,
parallel and bounded-feedback routes. Parallel keeps an independent.3-gain clean
branch; invalid/overload handling at this benchmark boundary is tested. The routing
is laboratory infrastructure, not a new patch format or production graph engine.
Hall/Cloud's64-frame adapters add64-frame buffering and burst workload; average
cost alone cannot admit them against a48-frame deadline.

Current main's production Engine exposes Bypass/Gain/Lowpass and at most one RAT;
most library voices are standalone. Thus “any drive + any verb + any modulation +
any delay” is an engineering target, **not demonstrated functionality**. We need
source-bound physical measurements of the whole combinations, memory/stack/cache
placement and reserve for I/O, switching and interrupts. An unknown combination
must remain unadmitted, rather than inherit an isolated effect's green status.

The founder-confirmed RAT maxima370915/270757/235475 cycles correspond to
77.27/56.41/49.06% of480000 cycles. They are preserved separately in
[reported physical evidence](reported-physical.json): unconnected input,
callback-body scope, incomplete ISR overhead and no supplied exact artifact binding.
Do not use them as worst-case certification or subtract bypass to invent isolated cost.

## Reproduce

From repository root; C++17 GNU/Clang on Linux, Python3 standard library. No CMake,
JSON download or third-party Python package is needed for the new host harness.
Exact source pins may require GitHub access on a fresh/shallow clone; use`--fetch`.

```bash
python3 -m unittest discover -s parable-starter/tests -p test_resources.py -v
python3 parable-starter/tools/resources/inventory.py --fetch --output source-inventory.json.gz
python3 parable-starter/tools/resources/run.py --output build/resources --fetch --chains --compile-only
# Run measurement after other compiler jobs stop; affinity/timer context is recorded.
python3 parable-starter/tools/resources/run.py --output build/resources --fetch --chains --reuse-builds
python3 parable-starter/tools/resources/arm.py --compiler "$ARM_GXX" --output build/resources-arm --fetch --chains
python3 parable-starter/tools/resources/collect.py --host build/resources/ledger.json --arm build/resources-arm/arm-ledger.json --output build/resource-report
```

`ARM_GXX` is the exact pinned14.2.1 compiler installed by the existing
`python3 parable-starter/firmware/build.py` recipe; use its verified libDaisy cache.
The new workflow screens **current-checkout** main kernels, not only historical
pins. Full-catalog and short-CI protocols are distinct and recorded. Existing DSP
regression/SPICE/audition suites were not redundantly rerun for unchanged algorithms.
Pre-edit core/audio-safety/Soundforge baseline5/5; focused infrastructure8/8,
including actual RAT/1x and ASan+UBSan on an actual mixed parallel chain. Exact
local logs/commands are retained in[verification](verification.json).

## Hardware handoff and unproven work

The kit contains15 distinct BOOT_NONE internal-flash images: four RAT builds,
CMOS, germanium, Muff, TS808, Klon, Blender, FZ1, tape saturation, spring, Hall,
and the Scorpio/Rooms/tape/rotary chain. Original SDK linker layout is unchanged;
no internal-flash enlargement or bootloader migration. Source identities, binary/
ELF hashes, actual linker sections, maps, symbols and stack files accompany them.
Link success does not prove boot/codec/SDRAM/analog compatibility or runtime budget.

Default images leave audio **stopped** and time48-frame kernels in the foreground.
Input generation is outside that timer; processing, stores and finite checks are
inside. Capture/bookkeeping and USB reporting follow it. They print a frozen
3000-routine report with RCC clock, FPSCR/FPDSCR/cache settings, native solver/containment counters and source/artifact
identity. Foreground timing is not callback timing. The separate callback option
measures its body including pattern generation/output writes; it remains untested.
Use the existing PAR009 first-boot kit's ROM-DFU recovery instructions, generated
by`firmware/build.py`. Nothing is flashed by any resource tool.

```bash
python3 parable-starter/tools/resources/firmware.py --compiler "$ARM_GXX" --libdaisy "$LIBDAISY" --output build/resource-images --only muff ts808 klon blender fz1 germanium tape_saturation spring
python3 parable-starter/tools/resources/hardware.py --manifest build/resource-images/artifacts.json --matrix measurements.csv
python3 parable-starter/tools/resources/hardware.py --manifest build/resource-images/artifacts.json --capture board-report.txt --output measured.json
python3 parable-starter/tools/resources/hardware_compare.py --manifest build/resource-images/artifacts.json --captures board-report-*.txt --output compared.json
```

No new physical captures, ISR-inclusive WCET, target initialization time, analog
sound, stack high-water, cache worst case, musical optimization approval or complete
chart admission has been established here.44.1/96kHz are not resource-tested by
this48kHz campaign. Only BOOT_NONE was built; option presence for other official
layouts does not establish their boot compatibility. Pinned unmerged sources must
remain fetchable or their source cache retained for long-term reproduction.

Next smallest engineering step: import the15 physical foreground captures, then
measure the light whole chain in callback context and actual stack/analog operation.
The first software optimization ticket should retain193-tap references and test
exact-order shared FIR traversal before any shorter-filter or numerical adoption.
