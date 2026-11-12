# PAR-012 Gate 0 — source control, before code

Base: current main `8f943681ad933f7d8b65d1d66535aa811c75c48e` (merged #33).
Read issue #34 in full, founder upload, #39 compatibility note, project AGENTS,
Constitution, README, implementation status, ADR-001, PAR-010 journal and PAR-011
protocol/implementation, canonical fixtures, prepare bridge, Engine and RAT.
No root AGENTS.md exists; the project rules are `parable-starter/AGENTS.md`.
Clean initial Debug/Release builds passed **23/23** (118.28 / 78.84 s, CTest -j2)
before edits. Exact commands and subsequent evidence: `PAR012_EVIDENCE.md`.

## Audit and smallest safe slice

Reuse canonical Codec -> journal save/load -> Runtime.stage/prepare/Engine.load
on control; Runtime.processBlock exactly once per audio block. The journal is
not an activation or modulation authority. Engine setters are unsynchronized;
Rat.configure builds the inverse table and resets state. Do not call any of these
from a changing-control callback. No Engine, circuit, canonical or firmware edits.

Add a separate fixed source scheduler, SPSC control FIFO and test-only audio
wrapper. Raw streams remain typed observations, never audio-rate sensor inputs.
Only **explicitly supplied normalized synthetic controls** can bind to the two
instrumentation destinations: pre-engine gain [0,2], post-engine gain [0,1].
No physiological feature extraction, relationship, artistic mapping or Body
Language operator. Features already supplied by a future adapter retain identity
and provenance; this increment validates them but does not invent their meaning.
Unbound measurements still exercise the complete ingestion/timing/quality lane.

## Chosen bounded contract

- Up to 16 declared source/channel profiles; explicit type/unit, origin
  (observed/synthetic), presentation (live/recorded), acquisition/delivery cadence,
  timestamp resolution, expected latency, background capability, TTL and consent
  expiry. No device discovery or claimed hardware connection.
- Records retain measurement AND monotonic arrival time, source timebase/epoch,
  sequence, validity, quality/confidence and permission. Unknown timestamps stay
  unknown. Missing/invalid is a status, never a zero physiological measurement.
- Explicit affine clock calibration (anchors, scale, uncertainty, bounded horizon).
  No inferred synchronization. Restart requires a new epoch/calibration, flushes
  that source, invalidates queued controls. Reject duplicates, backward source
  times/arrival, late committed input and stale/future measurements. No speculative
  jitter buffer: ordered replay and explicit rejection are sufficient here.
- Fixed 256-record host ingest FIFO; drop-new with counters. Fixed 32-snapshot
  SPSC control FIFO; Busy/drop-new with counters. No unbounded archival store.
  The caller may inspect/drain raw facts on control. No implicit recording/upload.
- Replay watermark: feed **all** records due at a control tick, in arrival order,
  before advancing that tick. Input chunking changes batching, not arrival times.
  Normal advance scans at most 256 records + 16 profiles. Simultaneous session
  expiry can additionally compact <=256 records per affected channel (<=16).
- Hold a valid normalized target through its measurement-based TTL; then return to
  neutral gain. Missing/bad quality/revocation/expired consent do not become beat
  events. Audio linearly slews bounded gain per sample; IBI events never interpolate.
- Serialized control producer, one non-reentrant audio reader; acquire/release
  uint32 indices only, no callback CAS/retry. Revocation/reset advances a monotonic
  session epoch, so queued old controls cannot revive. Audio independently checks
  TTL at boundaries if control stalls. Refuse counter/epoch exhaustion.
- Audio wrapper latches due snapshots at boundaries (<=32), slews samples, uses
  fixed scratch <=1024 frames, calls existing Runtime once, applies post gain.
  Global test envelopes persist across patch adoption; receipts retain the actual
  Runtime token. No per-processor mutable parameter access. Unity must match.
- Startup/restart/cleanup/destruction require external joined audio, as PAR-011.
  Stop uses Runtime's existing request/ACK. No timeout is evidence of quiescence.

## Source and consent gate

Official Apple/Samsung/BLE/Polar/Garmin/Wear OS/Empatica/Fitbit/browser documents
were consulted before code. `LIVING_SOURCE_REQUIREMENTS.md` will record the sourced
matrix, including Samsung acquisition versus batching and Apple restricted held
PPG. A 300 s HR fixture is a sparse *simulation*, not a claim about every current
Apple model. No native SDK, personal data, transport, permission bypass or upload.
Future remote adapters must authenticate/encrypt access and serialize revocation;
these declared host profiles are not an implemented security boundary.

No architecture blocker: composition around PAR-011 is sufficient. New persistent
mapping syntax, source adapters, guitar observation (#38), founder grammar (#39),
hardware lifecycle/deadlines and listening approval remain separate decisions.
