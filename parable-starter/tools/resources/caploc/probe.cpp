#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "capricorn_loc.h"
#include "probe_id.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <new>

extern "C" USBD_HandleTypeDef hUsbDeviceFS;
static daisy::DaisySeed hw;
struct Moderate : CapricornLoc { Moderate():CapricornLoc(.42f,false){} };
struct Leaky : CapricornLoc { Leaky():CapricornLoc(1,true){} };

using ModelStorage = std::aligned_union_t<0,Moderate,CapricornLoc,Leaky>;
static ModelStorage storage;
struct Result { unsigned blocks, peak, hits, bad, frames; };
static Result results[3]{};
static float patterns[4][256];
static volatile unsigned blocks, peak, hits, bad, badFrames;
static volatile float sink;
constexpr unsigned totalBlocks=3000;

template<class Model> static void audio(daisy::AudioHandle::InputBuffer, daisy::AudioHandle::OutputBuffer out, size_t frames) {
    const uint32_t begin=DWT->CYCCNT;
    auto* fx=std::launder(reinterpret_cast<Model*>(&storage));
    const unsigned block=blocks;
    for(size_t n=0;n<frames;++n) {
        if(block<totalBlocks) {
            float x=patterns[(block/32)%4][(block*48+n)%256];
            float y=fx->process(x);
            sink=y;
            if(!std::isfinite(y)) ++bad;
        }
        out[0][n]=out[1][n]=0;
    }
    __DSB();
    const uint32_t elapsed=DWT->CYCCNT-begin;
    if(block<totalBlocks) {
        if(frames!=48) ++badFrames;
        if(elapsed>peak) peak=elapsed;
        if(elapsed>=480000) ++hits;
        blocks=block+1;
    }
}

template<class Model> static void measure(Result& result) {
    auto* model=new(&storage) Model;
    blocks=peak=hits=bad=badFrames=0;
    hw.StartAudio(audio<Model>);
    while(blocks<totalBlocks) hw.DelayMs(1);
    hw.StopAudio();
    result={blocks,peak,hits,bad,badFrames};
    model->~Model();
}

// USB only selects the requested mode. Only the audio ISR owns the live DSP state.
static std::atomic<unsigned> requestedMode{2};
static std::atomic<unsigned> playingMode{0};
static CapricornLoc liveDrive;
static void receive(uint8_t* bytes, uint32_t* length) {
    if (*length == 1 && bytes[0] >= '1' && bytes[0] <= '3')
        requestedMode.store(bytes[0] - '0', std::memory_order_relaxed);
}

static void play(daisy::AudioHandle::InputBuffer in, daisy::AudioHandle::OutputBuffer out, size_t frames) {
    const unsigned mode = requestedMode.load(std::memory_order_relaxed);
    liveDrive.setRegister(mode == 1 ? .42f : 1.f);
    liveDrive.setLeaky(mode == 3);
    for (size_t i = 0; i < frames; ++i)
        out[0][i] = out[1][i] = liveDrive.process(in[0][i]);
    playingMode.store(mode, std::memory_order_relaxed);
}

int main() {
    hw.Init(true);
    hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);
    SystemCoreClockUpdate();
    const auto hz=SystemCoreClock;
    const bool seed3=hw.CheckBoardVersion()==daisy::DaisySeed::BoardVersion::DAISY_SEED_3;
    const unsigned rate=static_cast<unsigned>(hw.AudioSampleRate());
    CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR=0xC5ACCE55; DWT->CYCCNT=0; DWT->CTRL|=DWT_CTRL_CYCCNTENA_Msk;
    __DSB(); __ISB();
    auto first=DWT->CYCCNT;
    for(unsigned n=0;n<32;++n) __NOP();
    bool counter=DWT->CYCCNT!=first;
    if(seed3 && hz==480000000 && rate==48000 && counter) {
        uint32_t random=0x504152;
        for(unsigned n=0;n<256;++n) {
            random=random*1664525+1013904223;
            patterns[0][n]=.2f*float(int32_t(random))/2147483648.f;
            patterns[1][n]=(n/17)%2?.9f:-.9f;
            patterns[2][n]=.2f*std::sin(6.28318530718f*7971*n/48000);
            patterns[3][n]=n<128?.8f:-.8f;
        }
        measure<Moderate>(results[0]);
        measure<CapricornLoc>(results[1]);
        measure<Leaky>(results[2]);
    }
    static char msg[512];
    const auto& a=results[0]; const auto& b=results[1]; const auto& c=results[2];
    int len=std::snprintf(msg,sizeof(msg),"CAP_LOC id=%s seed3=%u core_hz=%lu rate=%u counter=%u moderate=%u,%u,%u,%u,%u exalted=%u,%u,%u,%u,%u leaky=%u,%u,%u,%u,%u live=0\n",
        PROBE_ID,unsigned(seed3),(unsigned long)hz,rate,unsigned(counter),
        a.blocks,a.peak,a.hits,a.bad,a.frames,b.blocks,b.peak,b.hits,b.bad,b.frames,c.blocks,c.peak,c.hits,c.bad,c.frames);
    hw.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
    hw.usb_handle.SetReceiveCallback(receive,daisy::UsbHandle::FS_INTERNAL);
    bool withinBudget = seed3 && hz == 480000000 && rate == 48000 && counter;
    for(const auto& row : results)
        withinBudget = withinBudget && row.blocks == totalBlocks && row.peak > 0 &&
                       row.peak < 96000 && !row.hits && !row.bad && !row.frames;
    if(withinBudget) hw.StartAudio(play);
    hw.SetLed(true);
    for(;;) {
        uint32_t mask=__get_PRIMASK(); __disable_irq();
        if(len>0 && size_t(len)<sizeof(msg) && hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData &&
           !static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData)->TxState) {
            msg[len-2] = static_cast<char>('0' + playingMode.load(std::memory_order_relaxed));
            hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(msg),len);
        }
        __set_PRIMASK(mask); hw.DelayMs(750);
    }
}
