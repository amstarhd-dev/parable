# SF025 Gate0 — revised Tiled Room brief

Current main767a3c402b972fc20df8aea0729adf794f4b7a38, independent sf-025 branch.
Read AGENTS/Constitution, README/status, ADR001, Soundforge recipe and full revised
assignment. Latest uploaded copy changes formatting only; supersedes the earlier
treated-studio-room/flutter concept. No old reverb/source or outstanding PR edited.
Before code: Release build core/audio-safety/shared utilities;5/5 pass,1.06s.

Primary research: Sean Costello,2009-12-22, original Valhalla article; original
WordPress mirror accessible when current site returns403. Findings paraphrased:
4ms predelay; small size; rapid dense onset; short decay around.62s; bright input
with frequency-dependent tail; no modulation. Eos is proprietary and its recipe
is not a complete PCM70 algorithm or source license. Original Lexicon PCM70 Rev2
owner's manual inspected through public mirror; Rich Chamber/Tiled Room4.2,
running/stopped reverb behavior distinguished. No firmware/EPROM/plugin code or
commercial IR imported. Manufacturer download page403 documented, not bypassed.
Authorized public demonstrations located (AMAZONA PCM70 review with hosted drum
examples, YouTube xiDudA83mfQ/yQTvOa-xSEg); no right to redistribute inferred,
no scraped/rehosted demo or controlled hardware-equivalence claim.

Candidates:
- Mike Jarmy dsp-lib PlateReverb.hpp, MIT,
  a26e67c8c9a12038cc51fdec5f7bed3941ce465b: complete stereo Dattorro figure8,
  input allpasses, tank diffusion/taps, size/damping. Select for compact static
  no-modulation tuning, consistent with Costello's plate-family reconstruction.
  Original dynamic maximum-size buffers are excessive; port same scalar signal
  flow/taps to bounded caller storage and remove the unused LFO. Validate directly
  against original at identical controls with modulation depth0. No new tank design.
- Signalsmith Basics369e906e03760ec219d07b06fa06dc4fd099f377, MIT; complete
  8line FDN, several diffusion stages, direct early contribution. Prior SF021
  runnable candidate reused for comparison with detune0/short size/decay. More
  framework and delay-array overhead than this task needs; not default choice.
- John Williamson dattoro_reverb b69fb9eb505a1bb4d31158674e4b63da38fad871,
  MIT: audited, not imported. Current P-loop never writes delay4453 before
  reading it; default modulation enabling also depends on stale extent; unsafe
  basis for a fidelity comparison without separate fixes. Do not silently repair.
- Dragonfly/Freeverb3 faf0322e8d3d44cdf998e19a037aacba25231dcc: useful
  Moorer/Rich Room prior art but GPL, audit-only. No GPL import or policy expansion.
- Mutable Rings/Clouds, MIT: efficient Griesinger family, but fixed engine lengths,
  embedded quantization and modulation need more adaptation for this target.
- Existing Parable Cancer CloudSeed is a large modulated cloud/chamber; Leo Hall
  is a long kCathedral network. Preserve unchanged. Existing Taurus plate is an
  unmerged independent experiment; do not merge or add its mechanical fault.

Smallest step: render borrowed healthy compact-room recipe, supply listening ZIP,
then write/implement one same-network fault only after that milestone. No new
framework, Engine/patch/firmware/astrology change. Human musical acceptance open.
