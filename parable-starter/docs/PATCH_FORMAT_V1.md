# parable.patch version 1

This is the PAR-002 control-side interchange contract selected in ADR-001 Option A.
It is independent of the old DSP prototype's internal `Patch::schema=2`.
`Codec::decode` returns either a new `ValidatedPatch` or `Error{code, field, rule}`.
Only successful validation constructs a public typed patch. Existing values are
never edited by decode. The result exposes const accessors and holds named
`Gain::linear` or `OnePoleLowpass::cutoffHz` settings with stable instance IDs.
Canonical JSON is the interchange authority; typed data is its derived value.
There is no adapter to the active audio engine in this increment.

## Required fields

Every object is closed: unknown fields reject. All fields below are mandatory;
there are no implicit playback defaults. Object key order, processor declaration
order and edge declaration order have no semantic significance.

| Object | Fields and rules |
| --- | --- |
| Root | `format`: `"parable.patch"`; `version`: integer 1; `sampleRate`; `outputTrim`; `processors`; `routing`. |
| `sampleRate` | Quantity `{ "unit": "Hz", "value": number }`; binary32 range [8000,192000]. This records the validation rate, not an automatic target-rate conversion. |
| `outputTrim` | Quantity with unit `"linear"`, range [0,1]. Separate from future performance Volume control. |
| Each processor | `id`: integer [1,4294967295], unique within the patch; `type`: one of the names below; `revision`: integer 1; `enabled`: boolean; `parameters`: object. |
| Bypass parameters | Exactly `{}`. |
| Gain parameters | Exactly `{"gain":{"unit":"linear","value":number}}`, range [0,2]. |
| OnePoleLowpass parameters | Exactly `{"cutoff":{"unit":"Hz","value":number}}`, cutoff >=20 and strictly < `0.45f * sampleRateHz`. Bounds match the existing native validation, not the old header's fixed-20000-Hz comment. |
| Each routing edge | Exactly `{"from":endpoint,"to":endpoint}`. A source is `"input"` or an existing processor's integer ID. A destination is an existing processor's integer ID or `"output"`. |

Type names are exactly `Bypass`, `Gain`, `OnePoleLowpass` (case-sensitive).
Units are exact, case-sensitive strings. `linear` means amplitude ratio, not dB
or UI percent. IDs/version/revisions require integer JSON tokens without decimal
points or exponents. Identity belongs to `id`, never type or array index.

Routing must be one input-to-output serial path visiting every declared processor
exactly once, including disabled processors. There are exactly N+1 edges for N
processors. Empty patches explicitly connect input directly to output. Duplicate
edges, branches, merges, dangling references, self-feedback, cycles and disconnected
processors reject. Each endpoint implies the sole mono port; explicit port/multibus
fields reject. Reordering requires editing edges; it does not renumber instances.

See [the manual example](../tests/fixtures/patch-v1/manual.json),
[all utility types](../tests/fixtures/patch-v1/utilities.json), and
[canonical output](../tests/fixtures/patch-v1/utilities.canonical.json).

## Numeric and canonical byte rules

- Playback quantities convert **directly from their original decimal token** via
  C++17 floating `from_chars` to nearest IEEE-754 binary32. This avoids double
  rounding through the JSON DOM's binary64 numbers. Bounds apply to the converted
  binary32 value, with no clamping; for example a decimal just above 2 that rounds
  to binary32 2 is accepted as 2. Values rounding beyond a bound reject.
- Non-finite values and floating conversion range errors (overflow or underflow
  to zero) reject. Representable subnormals round-trip. JSON NaN/Infinity literals,
  numeric strings and booleans are not numbers in this format. Negative zero
  normalizes to positive zero. Ordinary IEEE floating semantics/nearest rounding
  are required; fast-math and alternate floating environments are not verified.
- Canonical bytes are compact ASCII JSON, without BOM, whitespace or final newline.
  Keys are lexicographically ordered; processors sort by numeric ID; edges follow
  the serial path. All fields are written, including disabled/enabled flags.
- Quantities use `to_chars(scientific, precision=8)`: nine significant digits,
  lowercase `e`, explicit exponent sign and at least two exponent digits (e.g.
  `1.50000000e+00`). Zero alone is `0`. IDs and versions use unsigned decimal
  integers. This policy is format-specific, not RFC 8785 / generic JSON canonicalization.
- The guarantees are `decode(encode(p)) == p` for every supported playback field,
  and `encode(decode(canonicalBytes)) == canonicalBytes`. Other accepted JSON
  normalizes on its first write; original whitespace, order and number spellings
  are not retained. No chart/metadata fields are silently discarded: they reject.

Golden byte expectations were generated independently using Python's binary32
packing and fixed `.8e` formatting, then checked against the C++ codec. Explicit
halfway/just-above-halfway decimal cases exercise the conversion policy. Future
phone/other-language codecs must pass these same fixtures; none is built here.

## Limits, errors and execution boundary

Input limit: 16384 bytes, 8 nested containers (root counts as one), 512 JSON
values including containers, 64 decoded UTF-8 bytes per key/string, 64 bytes per
floating numeric token, 8 processors and 9 edges. These are version-1 safety
limits, not measured DSP/MCU resource budgets. Parsing enforces byte/depth/value/
string limits and rejects duplicate keys (including escaped equivalents) before
schema validation. JSON must be strict UTF-8; comments, trailing data, malformed
syntax, raw NUL bytes and BOM reject. Schema errors identify a field as `$` plus escaped JSON
Pointer segments, and name the failed rule. Array indices refer to input order.
Only the first error is returned; callers must check the result variant.

The codec allocates a bounded-input DOM and uses control-side error handling.
Resource exhaustion may propagate allocation exceptions without mutating prior
validated data. It must never run inside audio. Only `parable_patch` links the
JSON dependency; existing core/audio targets do not link or call it. ARM toolchain
support (including floating `charconv`), memory footprint and processing costs
remain unbuilt/unmeasured. A limit on JSON size is not proof of MCU fit.

Four independently held encoded strings are demonstrated in host tests. They
are RAM payloads; no bank-file format, nonvolatile storage, activation protocol,
compiler, UI, provenance, expression mapping or runtime preparation is implemented.
Future preparation must check the actual target sample rate and resource budget
before use; this contract does not authorize playing an incompatible patch.
