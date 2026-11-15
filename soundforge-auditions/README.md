# SF-015 listening and validation artifacts

This branch publishes validated artifacts for code PR61; **do not merge this artifact branch**. Native source commit fc9f3eb4d583bf8307f8a5900255976683e47373 remains the code PR head. No DSP or production files differ from that source; only this artifact directory is added.

Open `sf015/attack_mid_dry_common.wav`, `sf015/attack_mid_healthy_common.wav`, and `sf015/attack_mid_feed_open_common.wav`, then the two RMS-matched versions. `sf015/` contains104 WAVs, full attribution, raw/common/RMS receipts, original sources, independent fixtures/netlists, measured data/figures, logs and ARM object evidence. Verify `sf015/SHA256SUMS`. The separate sampling ZIP retains actual native sampling captures and timings.

GitHub's release-asset upload endpoint rejected this environment's credentials (HTTP401); normal authenticated Git publication works. This artifact-only branch provides the complete files through ordinary repository downloads and the release's generated source archive. The prepared standalone ZIP remains available in the originating shared workspace. No claim of successful binary release-asset upload is made.

Scientific acceptance remains incomplete:12/24 full-band RMS gates miss; all18 false-spur checks pass at8x. Abrupt healthy gating is not demonstrated; empirical2N270/pedal calibration, founder approval and physical Seed3 timing/execution are not established. This is measured-AC128-template original-FZ1-style research.

The artifact-only commit skips redundant CI; the separate code PR retains all required checks, including the enforcing scientific gate. Code CI currently fails at startup before any checkout/build/test.
