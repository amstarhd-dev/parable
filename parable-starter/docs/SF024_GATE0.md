# SF-024 Gate 0

Read complete supplied assignment, project AGENTS, Product Constitution, README,
implementation status, Soundforge methodology, ADR023/report and actual four
processors/graph/router. Start frozen PR78 head06e54dbf797c708f46a5890cf97a4c1839952d63;
main767a3c4. Stacked experimental branch; neither77 nor78 merged or edited.
Before changes: Release configure/build parable_wallace_test/parable_wallace_lab;
ctest -R '^wallace_' **2/2 pass**,1.74s. Existing Hall misleading-indentation
warnings retained. Exact local commands appear in final report.

Borrow first: HiFi-LoFi/FFTConvolver f2cdeb04c42141d2caec19ca4f137398b2a76b85,
MIT, portable Ooura backend; all six original selected files verified by pinned Git blob. Local follow-up: Utilities.h empty-buffer memset guard after an actual UBSan null-pointer diagnostic; upstream/local hashes and patch rationale retained.
Signalsmith Linear is a licensed FFT alternative but requires building more convolution
glue. No FFTW/JUCE imported. FFTConvolver already supplies uniform partitioned
zero-extra-latency processing with initialization allocation outside processing.
Ooura's original terms permit all uses/copy/modification/distribution (see notice).

IR selection: jesterdyne Freesound116743/116735, actual measured cabinet/SM57
responses, CC BY4.0 original pages; unchanged lossless bytes cross-checked against
OpenSauce/rustortion1522f4d64f520bd665cfb098601a2cbf0bcae0d8 and
Raven-deadwire/SpectralForge-Chimera-Amp-Matrix b900c9f7f028a74446ec65f11aa8a2e435c12ecb.
Originals44100Hz/mono24bit; titles Jensen/SM57 and ENGL Celestion V30/SM57.
Original generic H&K description conflicts with latter title: preserve title,
do not invent dimensions, cabinet geometry or independently authenticated capture.
Axion/Gleokit web inventories researched; direct download unavailable/403.
Darwinscat/Felitronics and Nave explicitly synthetic: rejected. Science Amps real
measurements lack explicit redistribution grant: rejected. No Freesound login bypass.
No credible existing drop-in amp in this native experiment; optional D omitted.
Do not invent a tanh amplifier or introduce plugin/model infrastructure.

Smallest change: cabinet processor/reference tests, then fixed pre/postcab recipes
using unchanged existing engines. Audio and meaningful default clean path first.
No processor, firmware, schema, correspondence or broader framework changes.
