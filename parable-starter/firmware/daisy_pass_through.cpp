// Daisy hardware adapter, v0.1. Requires the official libDaisy toolchain.
// NOT compiled/flash-tested in this release. No preset buttons or flash yet.
#include "daisy_seed.h"
#include "parable_core.h"
using namespace daisy;
static DaisySeed hardware;
static parable::Engine audioEngine(48000.f);
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    for (size_t i = 0; i < size; ++i) {
        // Two channels currently receive their own input. Mono guitar may use channel 0 only.
        const float left  = in[0][i];
        const float right = in[1][i];
        out[0][i] = audioEngine.process(left);
        // Intentionally copy left to right for a mono processing proof;
        // later two-channel audio needs separate channel state.
        (void)right;
        out[1][i] = out[0][i];
    }
}
int main() {
    hardware.Init();
    // The default libDaisy audio sample rate must be checked before using this adapter.
    audioEngine.load(parable::makeBypassPatch());
    hardware.StartAudio(AudioCallback);
    while (true) { }
}
