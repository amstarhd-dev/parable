// Build-only proof. Separate specimen, never production firmware or auto-flashed.
#include "daisy_seed.h"
#include "parable_tiled_room.h"
static daisy::DaisySeed seed;
static parable::soundforge::TiledRoom room;
// Exact48k requirement checked by prepare; not a claim about other-rate images.
static float history[12133];
void audio(daisy::AudioHandle::InputBuffer in,daisy::AudioHandle::OutputBuffer out,size_t frames){
 for(size_t i=0;i<frames;++i){const auto y=room.process(in[0][i]);out[0][i]=y.left;out[1][i]=y.right;}
}
int main(){seed.Configure();seed.Init();seed.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);seed.SetAudioBlockSize(64);
 if(!room.prepare(48000,history,12133,{.62f,.35f,true}))for(;;){}
 seed.StartAudio(audio);for(;;)seed.DelayMs(1000);
}
