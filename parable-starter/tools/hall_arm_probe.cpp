// Build-only standalone proof, never flashed by tooling. Not production firmware.
#include "daisy_seed.h"
#include "parable_hall.h"
#include <new>
using parable::dsp::Hall;
static daisy::DaisySeed seed;
// NOLOAD external state: construct only AFTER Seed::Init initializes SDRAM.
alignas(Hall) static uint8_t DSY_SDRAM_BSS storage[sizeof(Hall)];
static Hall* hall;
void audio(daisy::AudioHandle::InputBuffer in,daisy::AudioHandle::OutputBuffer out,size_t frames){
 if(frames==Hall::blockSize)hall->process(in[0],out[0],out[1]);
 else for(size_t i=0;i<frames;++i)out[0][i]=out[1][i]=0;
}
int main(){seed.Configure();seed.Init();seed.SetAudioBlockSize(Hall::blockSize);
 hall=new(storage)Hall;
 if(!hall->prepare(seed.AudioSampleRate(),{1,.35f,.98f}))for(;;){}
 seed.StartAudio(audio);for(;;)seed.DelayMs(1000);
}
