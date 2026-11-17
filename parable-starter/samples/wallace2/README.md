# Wallace II — listen first

**[Download the listening ZIP](SF024-listening.zip)**: ten matched comparisons,
three clean solos, three shared-gain controls and credits. Unzip and play locally.

Ten matched mono48k PCM16 WAVs,20s same Guitar-TECHS take03 +9s native tail.
Download the raw file in GitHub before playing; use headphones/monitors if available.
The clean guitar is present by default; it bypasses Muff/Rotary/Tape/Hall and
passes the common cabinet, with48sample nominal Muff latency alignment only.
All matched versions use measured−23LUFS, constant gain, no mastering dynamics.

| File | What changes |
|---|---|
| [01 Muff direct](01_muff_direct_matched.wav) | Original unchanged fuzz |
| [02 Muff + Jensen/SM57](02_muff_jensen_matched.wav) | Only cabinet1 added |
| [03 Muff + V30/SM57](03_muff_v30_matched.wav) | Only cabinet2 instead |
| [04 Original serial](04_original_serial_matched.wav) | Exact SF023 control |
| [05 Pre-Muff split / pre-cab ambience](05_pre_cab_clean_matched.wav) | **Default clean.50 + Muff.75**, clean-fed wet effects |
| [06 CLEAN-DISABLED CONTROL](06_CONTROL_clean_disabled_matched.wav) | Only direct clean removed; never the default |
| [07 More clean, less Muff](07_more_clean_less_muff_matched.wav) | Clean.75, Muff.45; other settings unchanged |
| [08 Post-cab studio ambience](08_post_cab_studio_matched.wav) | Same core gains/returns, cabinet before spatial effects |
| [09 Restrained ducking](09_post_cab_duck_matched.wav) | Only Tape/Hall ducking up to2.5dB added to08 |
| [10 Expansive](10_expansive_matched.wav) | Postcab, returns.25/.32/.40, clean still present |

For the clean-control question, compare05/06 matched first, then their
`_common_gain.wav` copies: that second pair uses **one identical gain**, exposing
the actual loss when clean is muted.07 also has this same-gain copy.

Solo proof at the exact fullmix listening gain (not boosted to sound impressive):
[05 clean before cabinet](05_pre_cab_clean_stem_clean_before_cab_listen.wav),
[05 clean after cabinet](05_pre_cab_clean_stem_clean_after_cab_listen.wav),
[08 clean after cabinet](08_post_cab_studio_stem_clean_after_cab_listen.wav).
Fuzz-aftercab and final-ambience solo stems are also provided for05/07/08/09/10.
`_raw.wav` versions retain identical unnormalized native gains. Individually
matched06 deliberately compensates its lost level; the shared-gain pair prevents
that compensation from hiding the difference. No wet-only principal output.

**Three listening questions remain open:**01/02/03 cabinet authenticity/usefulness;
05/06/07 clean articulation/composite;05/08 placement preference, then08/09 ducking.
These files supply the comparisons, not a fabricated human preference verdict.
No rock-recording quality, physical-pedal equivalence or Daisy readiness approval.

Original performances/IRs are licensed; see [guitar credit](GUITAR_NOTICE.md) and
[cabinet credit/change notice](IR_NOTICE.md). Exact IRs/preparation in irs/;
settings, raw levels, true-peak/LUFS logs, gains, stem/phase/reference errors in
analysis.json/levels.csv/settings.json. Original Muff and serial raw files match
SF023 byte-for-byte. Float intermediates are reproducible and omitted from Git.
Convolution1's small kernel timing is retained, not an artificial extra queue.
Full resource/tests/reproduction/limitations: [report](../../docs/SF024_REPORT.md).
