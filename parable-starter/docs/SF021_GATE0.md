# SF-021 Gate 0 — complete borrowed healthy engines first

Issue75, main767a3c402b972fc20df8aea0729adf794f4b7a38. Read AGENTS,
PRODUCT_CONSTITUTION, README, implementation status and Soundforge recipe.
PR48/72/74 are methodological references only; no source imported or branch merged.
Pre-edit Release utility/registry baseline3/3 pass,1.04s (log retained).

Compare complete pinned upstream audio before any new reverb/fault implementation:
CloudSeedCore deb21ded9eb7dad9b3ff94ce1ba96a963716594e MIT, supplied DarkPlate;
Signalsmith Basics369e906e03760ec219d07b06fa06dc4fd099f377 MIT reverb and supplied
ambient preset. Host audit calls actual processing, not rewritten mathematics.
Also inspect FFTConvolver f2cdeb04c42141d2caec19ca4f137398b2a76b85 MIT and actual
IR rights; convolution rights do not supply IR rights. No new reverb algorithm.

Signalsmith transitive audit: dsp v1.7.1 exact2d20161915e733f117545c6be8cd3275a739a1e3
MIT; linear0.6.4 de55e6a50ffcf6f8f43f649692d94691c7025151 MIT;
hilbert-iir1.0.0 9e42e09714c8215d51eaa1b52a3d54ae8dd2a3a3 0BSD.
Reverb directly includes dsp and bundled stfx-library.h; STFX explicitly declares
Boost Software License, despite the basics root MIT. linear/hilbert are umbrella
CMake dependencies, not direct reverb includes. Existing registry import allowlist
only MIT/BSD-2; do not silently mislabel BSL/0BSD as MIT. Research execution and
project admission are distinct; retain complete audit and propose any policy
extension separately, without weakening the validator in this task.

Freesound585740 author page verified: CC0, stereo48k24bit,.885s. Original download
requires login/OAuth; do not replace it with a lossy preview or bypass access.
OpenAIR alternative data/license verification ongoing; no unverified IR imported.

Gate1 is actual same-take healthy recordings + a short source-selection decision.
No source fame or nonzero tail establishes chamber quality. Do not claim founder
approval. Do not spend hours adding a fault to a core that fails this gate. If
healthy selection/rights/resources remain blocked, publish honest candidates and
unfulfilled criteria in one unmerged research PR; no invented fallback algorithm.
