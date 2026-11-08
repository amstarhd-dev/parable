// Seed3-oriented compile baseline. ARM-linked, NOT flashed/hardware-tested.
#include "daisy_seed.h"
#include "parable_core.h"
#include <optional>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Parable finite-input safety requires IEEE-aware floating point flags"
#endif
#ifdef PARABLE_CODEC_LINK_PROBE
// Diagnostic control-side link/size probe only; excluded from the minimal image.
#include "parable_patch.h"
#include "parable_prepare.h"
#endif
using namespace daisy;
static DaisySeed hardware;
static std::optional<parable::Engine> audioEngine; // Fixed storage; emplace before StartAudio.
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    for (size_t i = 0; i < size; ++i) {
        out[0][i] = audioEngine->process(in[0][i]);
        // Mono input 0 duplicated to both outputs; no stereo-state claim.
        out[1][i] = out[0][i];
    }
}
int main() {
    hardware.Init(true); // Request 480 MHz; actual clock/electrical operation unmeasured.
    hardware.SetAudioBlockSize(48);
    // This API reports configured SAI rate, not a physical clock measurement.
    const float rate = hardware.AudioSampleRate();
    audioEngine.emplace(rate);
#ifdef PARABLE_CODEC_LINK_PROBE
    const auto decoded = parable::patch::Codec::decode(
        R"({"format":"parable.patch","version":1,"sampleRate":{"unit":"Hz","value":48000},"outputTrim":{"unit":"linear","value":1},"processors":[],"routing":[{"from":"input","to":"output"}]})");
    const auto* document = std::get_if<parable::patch::ValidatedPatch>(&decoded);
    if (!document) while (true) {}
    const auto prepared = parable::preparation::prepare(*document, rate);
    const auto* native = std::get_if<parable::Patch>(&prepared);
    if (!native || audioEngine->load(*native) != parable::Status::Ok) while (true) {}
#else
    if (audioEngine->load(parable::makeBypassPatch()) != parable::Status::Ok) while (true) {}
#endif
    hardware.StartAudio(AudioCallback);
    while (true) { }
}
