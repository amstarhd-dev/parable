# SF-006 Gate 0 — before implementation

2026-11-12. Current main **0def50b8846a774a6bddb06e6bf9436598c95815** includes
approved merge #40. #42 is **open/unmerged**, head 057312b; its CMOS changes are
not imported or merged. Read AGENTS, Constitution, README/status, ADR-001 and
PAR-008/PAR-011 addenda, ADR-004, patch-v1, Soundforge recipe and firmware/resource
reports. Frozen Lexicon v1.0 and a root AGENTS.md are **not tracked**; no substitute
correspondence rules are invented. Issue #43 authorizes this sound experiment.

Fresh GNU 14.2.0 / CMake 3.31.6 baseline, repository root:

```sh
cmake -S parable-starter -B /workspace/work/sf006-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf006-Debug -j2
ctest --test-dir /workspace/work/sf006-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf006-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf006-Release -j2
ctest --test-dir /workspace/work/sf006-Release --output-on-failure --no-tests=error -j2
```

**26/26 each**, Debug 143.05 s / Release 78.66 s. No source edits preceded this
gate. Baseline test artifacts/hashes retained outside the checkout for comparison.

## Borrow-first audit and smallest path

No native delay/tape implementation or DaisySP dependency is tracked. Historical
browser/Python audio is not shipping DSP. DaisySP master pinned at
**2c72eaf9eac5fc0dca1919d65d606da907832618**: inspected MIT LICENSE,
`Source/Utility/delayline.h` and `dsp.h`. Reuse its small circular storage/reset,
one-pole and SoftClip idioms, with notices; don't import a whole library. Its
ordinary assignment at Write cannot retain old magnetic information. A dedicated
three-head medium update is needed; this is the actual original modeling gap.
CHOWTape 604372e4ffd9690c3e283362e4598cb43edbb475 is GPL-3.0, audit-only;
no code or curve fit imported. Full hysteresis/transport frameworks are beyond
this reduced erase/overwrite test. See ADR-006 and pinned provenance.

## Hard bounds and closed integration gate

Portable mono host specimen, integer-cell fixed transport, 44.1/48/96 kHz (and
existing WAV loader rates within 32–96 kHz); loop 0.1–2 s, replay delay >=20 ms,
erase-to-write spacing 15 ms. Round spacing to nearest sample and publish actual
geometry. At most **192,000 float cells / 768,000 B**, one externally supplied
buffer, plus **<=192 B processor** and **<=4 KiB ARM object text**. No hidden
second loop, per-cell age array, allocator or speculative cache. Clear/configure
outside audio; fixed work per sample, no solver or catch-up loops.

Prospective target admission ceiling: **1,000 M7 cycles/sample**, i.e. 48,000
cycles/48-frame block at 48 kHz, 10% of the configured 480 MHz. At 96 kHz the
same ceiling is 20%. This is a **hard future measurement gate, not an observed
WCET or a measured free budget**; current hardware CPU/timing remains unmeasured.
Host elapsed time and compiled operations will be reported separately.

Existing ARM Engine 11,040 B / four-bundle Runtime 45,224 B; canonical patch-v1
permits eight serial stages and one RAT, no variable buffer ownership. Adding
four worst-case media would require **3,072,000 B**, not the existing pool.
Pinned BOOT_NONE link region: 128 KiB FLASH, AXI 512 KiB, DTCM 128 KiB,
D2 288 KiB. Existing first-boot RAT load span 109,784 B; no tape linked there.
These are report/linker capacities, **not available RAM or measured deadlines**.
The linker has 64 MiB SDRAM at 0xc0000000 and SDK Init invokes SDRAM initialization,
but existing images allocate zero static SDRAM. No actual Seed3 memory access,
cache/MPU, DMA contention or SDRAM latency has been observed. QSPI is nonvolatile
storage/execution space, not interchangeable writable audio RAM.

**Do not integrate into Engine/patch-v1 in this increment.** Safe external-memory
ownership/placement across staging, active and retirement requires a separately
approved allocation/lifetime decision and hardware measurements. Do not change
saved bytes, Engine/RAT, four-slot journal, Runtime, firmware or living sources.
Deliver the finished host A/B, independent erase/overwrite reference, bounds,
sanitizers, actual ARM compile/layout and unchanged firmware links first.
