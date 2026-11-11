# PAR-010 Gate 0 — recorded before source edits

Base: main `156fa08bb06a2018270eda0a6cc44b09bb8ff869`, including merged PR #30.
Clean GCC 14.2 / CMake 3.31.6 baseline: 19/19 Debug (80.16 s), 19/19 Release
(20.14 s). Read AGENTS, Constitution, implementation status, README, ADR-001's
durable four-slot protocol, PATCH_FORMAT_V1, Codec sources/header, golden fixtures,
native Bank, CMake/tests, and third-party/Soundforge licensing rules. The root has
no AGENTS.md; the applicable one is inside `parable-starter/`.

## Prior art and choice

- [Zephyr NVS v4.3.0](https://github.com/zephyrproject-rtos/zephyr/blob/v4.3.0/doc/services/storage/nvs/nvs.rst):
  examined FIFO sectors, retain-before-erase, data then metadata, optional data CRC,
  and full/wear handling. Its RTOS/flash integration and variable-record collector
  exceed four fixed slots. Apache-2.0 implementation is reference only.
- [littlefs v2.11.2 design](https://github.com/littlefs-project/littlefs/blob/v2.11.2/DESIGN.md):
  examined two-block metadata pairs, revision/CRC commits and copy-on-write. A
  filesystem, splitting and wear allocator are unnecessary; BSD-3-Clause source
  is not imported under the project's conservative MIT/BSD-2 allowlist.
- [bakercp CRC32](https://github.com/bakercp/CRC32/tree/60f89f3b78de2cad300710adc8351bfcf7dc8346),
  MIT: inspected source and notice. Reuse its small 16-entry nibble CRC evaluator;
  remove Arduino/PROGMEM coupling and arbitrary native-struct overloads. Pin source,
  preserve license/provenance, check against independent Python zlib vectors.

Choose **two fixed erase-independent banks per slot**, one complete record per
bank. This is bounded A/B copy-on-write, with no filesystem, collector or new flash
manager. Reuse Codec for validation/canonical bytes; native Bank is RAM-only and
must not become storage authority. No patch/core/RAT changes are needed.

## Concrete protocol and boundaries

Eight banks reserve the full 16,384-byte payload capacity. Supplied geometry sets
erase and program alignment; no Seed address/partition is chosen. Use an explicit
64-byte little-endian envelope (magic, journal major, slot, nonwrapping uint64
generation, length, kind, payload CRC, header CRC), aligned payload, and a dedicated
final program granule containing the commit indicator. Erase/verify the inactive
bank, program/verify the envelope and canonical document, then program/verify the
indicator last. No multi-byte atomicity assumption. Old bank remains untouched.
For example 4,096-byte erase / 256-byte program needs 20,480 bytes per bank,
163,840 total. This is a host profile, not a Daisy memory-map decision.

Recovery checks bounds, CRC, commit and Codec/canonical parity before returning an
owned document. Expose empty/interrupted/corrupt/default/recovered/unsupported and
IO statuses. Committed unsupported versions block automatic overwrite/downgrade;
equal conflicting generations fail closed. At UINT64_MAX refuse further writes.
Clear is a separately journaled tombstone, not selection/activation or secure erase.
A cut after the complete marker but before acknowledgment can recover the new
record: return uncertain completion, never false success. CRC is detection, not
authentication or a guarantee against every multi-bit corruption.

Single serialized control owner; synchronous adapter IO must be durable on success
and preserve other erase regions. The simulator will cut erase/program at byte and
operation boundaries, preserve bytes across recreation, inject corruption and
wear failures. Exhaustive small-record cuts plus max-input/large-shape tests will
run with unchanged full regressions and targeted ASan/UBSan. No architectural
blocker found. Real geometry/atomicity, loader/QSPI reservation, endurance policy,
execute-vs-write restrictions and whether audio must stop remain hardware gates.
No firmware driver, Engine load, live activation or hardware claim in this ticket.
