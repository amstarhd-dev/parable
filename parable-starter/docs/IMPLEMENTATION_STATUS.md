# PAR-001 implementation status and reproducible baseline

Recorded: 2026-11-08. Inspected baseline commit:
`a47396d6037e33d635d128f42dc6c9c7c1718273` in `amstarhd-dev/parable`.
This is an engineering baseline and proposed decision, not release readiness.

## Inventory and instructions

All 11 files tracked at the inspected commit were read. Paths below are relative
to the Git repository root; the CMake project lives inside `parable-starter/`.

| Tracked file | Role and limits |
| --- | --- |
| `README.md` | Two-line repository description; no build instructions. |
| `parable-starter/AGENTS.md` | Agent/process rules; requires a baseline before code changes and a proposed two-option ADR. |
| `parable-starter/PRODUCT_CONSTITUTION.md` | Proposed product purpose, pilot scope, gates and founder decision rights; specification, not completion evidence. |
| `parable-starter/README.md` | Brick 0.1 build instructions and explicit prototype limitations. |
| `parable-starter/docs/CODEX_KICKOFF.md` | Initial task and historical status; incorrectly claimed a CI file was present. |
| `parable-starter/docs/HARDWARE_SUPPLY_DECISION.md` | Dated Seed3/carrier/supply proposal; no hardware measurements. Its vendor/stock/price assertions were not reverified in PAR-001. |
| `parable-starter/CMakeLists.txt` | C++17 host executable, warning flags, and the sole CTest entry `core_validation`; does not build firmware. |
| `parable-starter/include/parable_core.h` | Entire header-only portable core: types, validation, four-slot RAM bank, stopped-audio loading, mono gain/filter processing, volume and clamp. |
| `parable-starter/tests/test_core.cpp` | Only automated test source: one assert-based smoke executable. Coverage is detailed below. |
| `parable-starter/firmware/daisy_pass_through.cpp` | Uncompiled libDaisy adapter: hardcoded 48 kHz engine, default hardware initialization, left input processed and duplicated to both outputs; right input unused. |
| `parable-starter/firmware/Makefile.example` | Integration example requiring separately built libDaisy and ARM tooling; not a working project build target in this checkout. |

The four requested instruction files are present under `parable-starter/`;
none was fabricated or replaced. There are no nested agent instructions.
The claimed `.github/workflows/host-tests.yml` was **missing**, and no workflow
was tracked anywhere in the baseline. GitHub's Actions runs endpoint returned
`total_count: 0` before changes. No existing CI failure was available to diagnose.
The complete astrological correspondence rules, historical browser v3.3 and
Python renderer are not tracked here. Their correctness cannot be verified from
this source set; no such rules or historical implementations were introduced.

Instruction alignment findings:

- `AGENTS.md` calls the product an embedded guitar-effects product; the
  constitution already centers correspondence. PAR-001 proposes constitutional
  clarification that effects DSP enables correspondence, and keeps this wording
  discrepancy visible rather than rewriting the agent rules.
- Snapshot immutability, atomic activation and safe levels in the rules are
  requirements. Current copied structs and stopped-audio `load` only partially
  satisfy them; the prototype is not a concurrent patch authority implementation.
- The kickoff's suggested ADR filename is superseded by the user's requested
  `docs/adr/ADR-001-patch-authority.md`; its link is updated, without a second ADR.
- The hardware supply document describes a future carrier more definitively
  than the current physical-form investigation. Treat that as a dated proposal;
  carrier, guitar mounting and manufacturing form remain unvalidated. No board
  or mounting assumptions were added to the audio architecture.

## Build and test evidence

A fresh shallow clone of the private repository was made at the baseline commit.
`git status --short` was empty before configure. No tracked source changes were
made before the initial build and tests, and results were reported before edits.

Host: Linux x86_64, GNU C++ 14.2.0, CMake/CTest 3.31.6, Unix Makefiles.
CMake was initially absent. Environment-only setup (not a project dependency):

```bash
python3 -m venv /workspace/work/baseline-tools
/workspace/work/baseline-tools/bin/python -m pip install 'cmake==3.31.6'
export PATH=/workspace/work/baseline-tools/bin:$PATH
```

Reproduce from a clean checkout with Git, CMake >= 3.15, Make and a C++17 compiler:

```bash
git clone https://github.com/amstarhd-dev/parable.git
cd parable
git checkout a47396d6037e33d635d128f42dc6c9c7c1718273
cd parable-starter
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Private repository authentication is required. To verify the PR revision instead,
check out its commit before configuring; no root-level CMake project exists.
Debug matters: tests use `assert` for checks **and setup calls**. `NDEBUG` in a
Release build removes both and can yield a misleading pass. Baseline compiler
flags were `-g -std=gnu++17 -Wall -Wextra -Wpedantic`, with no `NDEBUG`.

Actual initial results (all three commands exited 0):

```text
-- The CXX compiler identification is GNU 14.2.0
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features - done
-- Configuring done (0.3s)
-- Generating done (0.0s)
[ 50%] Building CXX object CMakeFiles/parable_core_test.dir/tests/test_core.cpp.o
[100%] Linking CXX executable parable_core_test
[100%] Built target parable_core_test
    Start 1: core_validation
1/1 Test #1: core_validation ..................   Passed    0.00 sec
100% tests passed, 0 tests failed out of 1
Total Test time (real) =   0.00 sec
```

No compiler warnings were emitted. This is the entire existing automated suite,
not a firmware build or a musical test. The test executable prints
`PARABLE_CORE_V0_1: host validation passed`.

## What the existing test actually exercises

- All four slots initialize empty. Setting slot 0 leaves slot 1 empty, and
  mutating the source afterward leaves the saved stage value unchanged. It does
  not write/recall four distinct patches or exercise storage across restarts.
- An invalid gain edit returns `InvalidParameter` and preserves the prior gain.
  Wrong schema and unknown processor type reject. Selecting slot 4 rejects;
  selecting slot 0 succeeds. `Bank::get` itself has no bounds check.
- A gain/low-pass chain produces a finite positive first sample in a loose range.
  Swapping those linear processors gives an approximately equal first sample.
  Despite an earlier comment about stage order, this is **not** proof that
  noncommuting routing or meaningful topology changes work.
- Empty-patch dry processing, a scalar volume setting, output clipping on input
  9.0, and NaN input returning zero are checked. Filter response, long-run
  stability, aliasing, level matching and musical quality are not checked.
- `Engine::load` rejects an unknown type. The test does not assert preservation
  of all runtime state after rejected loads or exercise any concurrent operation.

## Functionality by evidence stage

“Specified” means a documented requirement; “implemented” means code exists.
“Host-tested” refers only to observed checks, not all possible behavior.
Hardware-tested and musically auditioned are independent evidence stages.

| Functionality | Specified | Implemented | Host-tested | Hardware-tested | Musically auditioned |
| --- | --- | --- | --- | --- | --- |
| Versioned canonical patch authority | Yes | Internal `schema=2` struct only; ADR proposed | Schema mismatch rejection only | No | No |
| Processor instances and routing | Yes | Up to 8 ordered stages; no stable IDs or edges | Two-stage linear-chain smoke only | No | No |
| Parameters and units | Yes | Per-type anonymous float and bounds; sample-rate-dependent cutoff | Invalid gain/type/schema cases only; not full boundary/NaN matrix | No | No |
| Four independent presets | Yes | Four RAM values and selected index; selection does not load Engine | Defaults/source-copy isolation, slot 0 vs 1, select bounds only | No | No |
| Durable save/load and round trips | Yes | No codec, flash journal or persistence; image-status enum values are unused | No | No | No |
| Immutable runtime snapshot / live activation | Yes | Engine copies a patch while audio is stopped; no exchange protocol | Synchronous load smoke only | No | No |
| Gain / one-pole low-pass / dry path | Utility plumbing | Yes, mono; hard output clamp | First-sample chain and dry/volume/clamp/NaN smoke | No | No |
| Register, master Volume, performance bypass | Yes | Scalar engine volume and empty-patch dry path only; no Register or physical controls | Basic volume/dry checks only | No | No |
| Safe output under all accepted inputs | Yes | Finite-input check and final clamp, with a known overflow gap below | Limited smoke passes; diagnostic gap reproduced | No | No |
| Traditional Western astrology compiler/trace | Yes | None | No | No | No |
| Phone-first Workshop composition/editing | Yes | None | No | No | No |
| Biometric and other grammars | Future extension | None | No | No | No |
| Standalone saved playback | Yes | Uncompiled startup-bypass adapter; no saved patch loading | No | No | No |
| Daisy Seed3 integration | Initial target | Demonstration adapter only; no verified toolchain/sample-rate setup | Not built by host CMake | No | No |
| Circuit-modeled effects / musical environments | Pilot requirements | None; gain/filter are plumbing | No musical/model validation | No | No |

No functionality is hardware-proven, auditioned or release-ready by this audit.
CPU load, worst-case callback time, heap/stack use, target memory use, latency,
noise floor, power consumption and switching transients: **not measured**.
Fixed array capacities seen in source do not establish measured target budgets.
The callback has no obvious allocating/I/O calls in source; no allocation or
real-time instrumentation was run.

## Minimal CI and baseline changes

| Changed file (repository-relative) | Reason |
| --- | --- |
| `.github/workflows/host-tests.yml` | Add the missing host workflow where GitHub discovers it; checkout, run in `parable-starter`, configure Debug, build, run CTest with an error if no tests are found. Push and PR triggers, read-only contents permission, five-minute timeout. |
| `parable-starter/.gitignore` | Ignore the documented local `/build/` output so the baseline remains reviewable. |
| `parable-starter/README.md` | Specify correct working directory, Debug/assert requirement, and links to evidence and ADR. |
| `parable-starter/docs/CODEX_KICKOFF.md` | Correct missing-CI history and link to the requested ADR location. |
| `parable-starter/PRODUCT_CONSTITUTION.md` | Proposed minimal purpose/phone/standalone/future-grammar/form-factor clarifications, explicitly subject to founder review. |
| `parable-starter/docs/IMPLEMENTATION_STATUS.md` | Inventory, reproducible evidence, stage separation, known gaps and next ticket. |
| `parable-starter/docs/adr/ADR-001-patch-authority.md` | Exactly two concrete alternatives, recommendation and implementation gates; no architecture implemented. |

No C++ source, tests, CMake targets, effect topology or astrological rule changed.
CI execution evidence for the PR is recorded in its description/checks, including
run links and any failures; a local pass is not described as a GitHub Actions pass.

## Unresolved risks and missing tests

- **Non-finite output from finite input:** an additional disposable host diagnostic
  found that a valid eight-stage gain patch (gain 2 each), `outputTrim=0`, and
  `std::numeric_limits<float>::max()` input overflows internally; infinity times
  zero becomes NaN, which the final comparison-based clamp does not remove.
  Observed output: `load_ok=1 output_is_finite=0`. This contradicts the source
  comment claiming always-bounded output. It is not a claim that normal guitar
  input reproduces it. Source was left unchanged to keep this baseline/proposal
  focused; output safety needs a small dedicated regression/fix before expanded
  DSP or hardware acceptance. Reproduce from repository root:

  ```bash
  c++ -std=c++17 -I parable-starter/include -x c++ -o /tmp/parable-finite-probe - <<'CPP'
  #include "parable_core.h"
  #include <cmath>
  #include <iostream>
  #include <limits>
  int main() {
      parable::Patch p;
      p.count = parable::kMaxStages;
      for (auto& s : p.stages) s = {parable::Type::Gain, 2.f, true};
      p.outputTrim = 0.f;
      parable::Engine e(48000.f);
      const auto status = e.load(p);
      const auto y = e.process(std::numeric_limits<float>::max());
      std::cout << "load_ok=" << (status == parable::Status::Ok)
                << " output_is_finite=" << std::isfinite(y) << '\n';
  }
  CPP
  /tmp/parable-finite-probe
  ```

- **Concurrency:** `Bank::set` is validate-then-copy, not a thread-safe atomic
  operation. `Engine::load`, `setVolume`, and `Bank::get` cannot be raced with
  their readers. The engine's copied configuration is private, but immutable
  snapshot lifetime/reclamation is not implemented. No race tests were run.
- **Validation:** no exhaustive bounds/type/sample-rate/count tests, parser
  fuzzing or serialization corrupt-image/version tests exist. `Bank::get` needs
  a valid index; the low-pass header's 20..20000 Hz comment differs from actual
  validation (< 0.45 * sample rate). Neither gap was silently corrected here.
- **Firmware:** not compiled, linked, flashed or connected to physical audio.
  libDaisy/toolchain version compatibility and actual sample rate are unverified.
  No physical switches, analog front end, supply behavior or mounting evaluated.
- **Persistence and activation:** no power-cut, storage wear, four-slot round-trip,
  invalid-live-edit, switching continuity, lock-free primitive or resource-budget
  tests. Stopped-audio patch replacement may reset filter history/click.
- **Product and music:** no real guitar, audio render/listening artifact, chart
  fixture, phone browser/usability test, circuit comparison or demand validation.
  Constitution amendments and ADR are proposals; neither approves itself.

## Proposed next engineering ticket

**PAR-002 — Minimal canonical patch codec and validation fixtures**, contingent
on founder approval of ADR-001. Implement only the chosen portable value/codec
contract for the three existing utility types, stable IDs and explicit serial
routing. Require deterministic canonical round trips, precise invalid-input
errors, four independent serialized slot payloads and equal manual/chart-like
fixture outputs. Do not add actual correspondence compilation, runtime exchange,
flash integration or effects. The ADR contains the acceptance details.

The smallest separate safety follow-up is a regression and bounded-output fix
for the overflow example above; it does not depend on approving the new patch
architecture and should precede claims of audio safety. After codec approval and
tests, isolate activation ownership and durable journaling into later tickets.
