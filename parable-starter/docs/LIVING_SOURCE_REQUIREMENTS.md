# Living source requirements — PAR-012

Experimental host infrastructure; **not wearable integration or an April pilot
dependency**. [Gate 0](PAR012_GATE0.md) preceded code. [Reproduction/evidence](PAR012_EVIDENCE.md)
separates host and compiler findings from hardware/listening approval.
[Retrieval manifest](research/PAR012-sources.json) records UTC retrieval times,
URLs/body hashes and failures. No measurement during guitar playing,
background endurance, real delivery latency, dropout or battery use was measured.

## Capability and timebase matrix

Acquisition Hz, API delivery cadence, physiological event rate and timestamp
precision are different. Values below describe documented capabilities, not our
observations. Consent, OS/model support and distribution gates still apply.

| Platform/path | Available facts and physical units | Acquisition / delivery / timestamp / freshness limits |
| --- | --- | --- |
| Apple Watch older background HR; active workout [1,2] | Processed HR (BPM), not beat events or optical waveform. HealthKit/workout permission. | Older opportunistic background readings may be minutes apart; 300 s is a simulation profile. Workout builder supplies collected measurements; no universal callback Hz or guitar-motion reliability guarantee. HealthKit dates need explicit mapping to monotonic control time. |
| Series 12 / Ultra 4 [1,3] | Apple advertises background HR as often as 5 s; Series 12 HRV as often as 5 min. | Hardware/product frequency does not grant third-party live per-beat or raw PPG access. Callback latency/coverage are unmeasured; do not generalize older 300 s assumptions to these models. |
| Apple saved heartbeat series, ECG, HRV/respiration/temp summaries [4] | Saved heartbeat-series query, recorded ECG and processed summaries are distinct types, not interchangeable live waveforms. HRV statistic/window and temperature meaning must be identified by adapter/channel. | A saved series is not a live beat subscription; gaps must remain gaps. Respiratory summaries do not supply breath phase. Irregular rhythm notifications are episodic events, not continuous per-beat quality. |
| Apple SensorKit optical PPG [5] | Research-entitled recorded optical samples; separate research permission and sensor support. | Documented **24-hour hold**. Direct ordinary-watch commercial live raw PPG is not established; see [PAR-WEAR-001 PR #37](https://github.com/amstarhd-dev/parable/pull/37). No private access attempted here. |
| Apple WatchConnectivity / iPhone browser [6,7] | Native watch/phone message transport; no assumption of Safari BLE capture. | Immediate messages require reachability; background transfers queue. Safari/iOS has no native Web Bluetooth (extensions are separate installations). Arrival time cannot stand in for measurement time. |
| Samsung Galaxy Watch Sensor SDK [8,9,10] | Continuous PPG 25 Hz (vendor raw units), acceleration 25 Hz, processed HR/IBI reports 1 Hz. IBI list contains 0–4 ms intervals and individual statuses; not one beat per report. EDA 1 Hz Watch8+, skin/ambient temperature Watch5+ (processed, cadence unverified). | Permissions plus partner release gate; developer mode is for testing. Display-off data may batch: official example **600 HR samples / 10 min**, with explicit flush. Timestamp spacing may vary even with nominal Hz. Validate background delivery on actual OS/model. On-demand ECG 500 Hz/PPG 100 Hz/SpO₂ are separate short foreground acquisitions, not continuous access. |
| BLE Heart Rate Service [11] | BPM and optional RR in 1/1024 s, potentially multiple intervals per notification. | Typically ~1 Hz notifications, server dependent; no original measurement timestamp field. Preserve receive-only time as unknown measurement time; do not invent event times or omitted beats. |
| Polar H10 SDK [12] | HR/RR ~1 Hz reports (ms); ECG 130 Hz (µV); acceleration 25/50/100/200 Hz (mG). | Native BLE adapter and device verification required. Used only as a heterogeneous rate reference, not a substitute for requested watch waveform access. |
| Garmin / Wear OS [13,14] | Device-queryable high-frequency motion/optional heartbeat intervals; independently timestamped Wear OS streams. | Capability dependent. Wear OS passive batching is unpredictable; exercise/measurement paths differ. No universal live rate promise. |
| Empatica E4 / Fitbit archive [15,16] | E4 recorded BVP 64 Hz, motion 32 Hz, EDA/temp 4 Hz, intermittent IBI. Fitbit intraday HR detail 1 s/1 min when available. | Research/archive examples, not watch integrations. File/cloud retrieval is not a live transport. Higher requested detail does not establish actual coverage. |

## Contract and faithful processing

`include/parable_living.h` defines portable fixed values; **not a wire encoding**.
Each record carries source/channel ID, kind/unit, measurement and arrival µs,
timebase/epoch, sequence, validity, quality, optional confidence, permission,
synthetic/observed origin and live/recorded presentation. Delivery delay is
arrival minus explicitly calibrated measurement time **only when known**.
An archival source remains `Recorded` even when its replay timeline is shifted.

Profiles declare capabilities/cadences, timestamp resolution, expected latency,
background support, TTL, clock calibration and explicit opt-in/session expiry.
These declarations are not evidence of device access. HR estimates, timed IBI,
HRV summaries, optical/electrical samples, respiration, XYZ acceleration, EDA,
temperature, supplied derived features and normalized controls stay distinct.
Units are explicit; future adapters must convert vendor units deliberately.
No HR→IBI synthesis, beat filling, feature extraction, medical/emotional inference,
Body Language relationships or musical grammar is implemented.

One serialized control owner registers <=16 channels; up to 256 owned raw records
wait in an arrival-ordered FIFO. Caller input can be reused after ingest. Duplicate
sequence, backward measurement/arrival, wrong epoch/unit/shape, corrupt/nonfinite
values, stale/future measurement and input at/before a committed watermark reject
explicitly. Sequence gaps are counted. There is no reorder buffer: future adapters
must order data or accept rejection. A control-side observer receives **every**
consumed event in order (including batched IBI); the scheduler retains only latest
facts and counters, not an implicit archival recording. Overflow drops new input.

Clock mapping is explicit affine calibration with finite scale [0.99,1.01],
uncertainty and <=24 h horizon. It is not inferred from arrival jitter. Unknown
measurement times are preserved but unavailable for fresh control. A source restart
requires a strictly newer epoch and calibration, flushes its queued observations
and invalidates old controls. Recorded time rebasing must be explicit. Calibration
before its anchor/beyond its horizon is rejected; refresh requires a new epoch.

Replay feeds **all** observations with arrival <= tick before `advance(tick)`;
ticks strictly increase. File chunks cannot change arrival times/watermarks.
Smoothing follows control selection, not a fictitious resampling of physiology.
The host demo uses 100 Hz control / 48 kHz audio / 48-frame blocks; these are
independent rates. Profiles provide TTL; fresh means measurement age plus clock
uncertainty < TTL and consent unexpired. Missing, invalid, poor/unknown quality
or stale controls return to neutral. Unknown is not a zero sensor value.

## Audio ownership and test-only destinations

Only supplied normalized **Control** records with good quality can bind; duplicate
destination ownership rejects, with no mixing/priority inference. All raw and
supplied Derived records remain unbound. The two global instrumentation mappings
are pre-engine gain `2*x` in [0,2] and post-engine gain `x` in [0,1]. Neutral is
pre=1/post=1. Neither is serialized into patch-v1 or a founder correspondence.

Control publishes <=32 fixed snapshots by aligned lock-free uint32 acquire/release
indices. Full returns Full/drop-new, not a wait. Audio consumes at most 32 due
snapshots per boundary; future snapshots wait. It linearly slews gains per sample
with maximum step `2/(.02*sampleRate)` (20 ms for gain span 0..2; 10 ms for 0..1).
This is causal slew interpolation, not invented intermediate beats or measured
high-rate physiology. TTL is independently checked on audio boundaries if control
stalls, with at most one block of extra holding. Blocks <=1024; caller supplies
monotonic boundary time and the **same actual rate** as Runtime. Backward/repeated
nonempty-block time falls back to neutral with an explicit time error.

The fixed wrapper scratch buffers precondition input, call PAR-011 Runtime exactly
once, then apply post gain. No Engine setters, circuit configuration, patch reload,
parse, file/network access, allocation or lock enters audio. Patch receipts keep
the actual active token; the global test envelope persists across patch adoption.
Unity matches the unchanged Engine, including rejected inputs and numerical rails.
Revocation/reset increments a session epoch: old FIFO entries cannot revive at the
next observed boundary. A racing mid-block revocation takes effect next boundary,
then gain slews to neutral. This is not instantaneous silence or a clinical alarm.

PAR-011 stop request/ACK, external audio join/quiescence, then Runtime cleanup +
Lane/Audio `resetAfterJoin` remain mandatory before restart/destruction. Recreate
Scheduler for a new replay timeline, or retain its strictly increasing control
time. Resetting a FIFO is never allowed with a reader in flight. Counters refuse
wrap; join/reset FIFO indices before 2³² publications (~497 days at 100 Hz).
Session epochs never reset; UINT32_MAX disables further control publication.

No personal sources, SDKs or uploads exist. Demo file output requires explicit
`--output`. Sensor data records cannot grant consent: explicit owner permission
changes control the session. Synthetic scenario files include separate test grant/
revoke actions. Future remote adapters require authenticated, encrypted, authorized
and revocable access **before** ingest; this host API is not an authentication layer.

## Fixtures and open gates

`tools/living_fixture.h` supplies transparent artificial waveforms and profiles:
300 s / 1 Hz HR; irregular IBI (0.5–3 events/s); five-second HR batches; 4 Hz EDA;
50/200 Hz XYZ; 130 Hz ECG; 25 Hz PPG; HRV/respiration/temp/supplied feature values.
All are **synthetic recorded test stimuli**, not observed device or derived medical
measurements. `trace-a.csv`/`trace-b.csv` are explicitly enumerated normalized test
controls, with missing/poor-quality periods, dropout and consent actions. Their
timing, units and generation are inspectable. Units/enums are in the public header;
`State` 0..5 = Missing/Fresh/Stale/Invalid/NoPermission/ClockReset.

Next smallest engineering step: PAR-013's neutral guitar-input observations (#38),
independent of physiology. Later decisions must specify persistent mapping schema,
actual adapter/clock/consent evidence, per-processor automation safety and target
memory/lifecycle/deadlines. Founder alone defines Body Language and listening
acceptance. No production firmware/DSP, astrology or patch-format change here.

## Sources

1. [Apple current HR behavior](https://support.apple.com/en-ca/120277).
2. [HKLiveWorkoutBuilder](https://developer.apple.com/documentation/healthkit/hkliveworkoutbuilder).
3. [Apple Series 12](https://www.apple.com/uk/apple-watch-series-12/).
4. [Saved heartbeat query](https://developer.apple.com/documentation/healthkit/hkheartbeatseriesquery).
5. [SRFetchRequest holding period](https://developer.apple.com/documentation/sensorkit/srfetchrequest).
6. [WCSession](https://developer.apple.com/documentation/watchconnectivity/wcsession).
7. [Browser compatibility](https://caniuse.com/web-bluetooth).
8. [Samsung sensor specifications](https://developer.samsung.com/health/sensor/guide/data-specifications.html).
9. [Samsung batching/flush](https://developer.samsung.com/health/sensor/guide/tracking-data.html).
10. [Samsung SDK/access](https://developer.samsung.com/health/sensor/overview.html), [IBI/status units](https://developer.samsung.com/health/sensor/api-reference/com/samsung/android/service/health/tracking/data/ValueKey.HeartRateSet.html).
11. [Bluetooth HRS 1.0 §§3.1.1.4–5 / 3.4](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/HRS_v1.0/out/en/index-en.html).
12. [Official Polar H10 rates/units](https://github.com/polarofficial/polar-ble-sdk/blob/master/documentation/products/PolarH10.md).
13. [Garmin Sensor API](https://developer.garmin.com/connect-iq/api-docs/Toybox/Sensor.html).
14. [Wear OS client/device differences](https://developer.android.com/health-and-fitness/health-services/compatibility).
15. [Empatica E4 file signals](https://www.empatica.com/blog/decoding-wearable-sensor-signals-what-to-expect-from-your-e4-data/).
16. [Fitbit intraday API](https://dev.fitbit.com/build/reference/web-api/intraday/get-heartrate-intraday-by-date-range/).
