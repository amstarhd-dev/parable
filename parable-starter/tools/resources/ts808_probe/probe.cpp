#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "reduced.h"
#include "probe_id.h"
#include <cmath>
#include <cstdio>
#include <new>

extern "C" USBD_HandleTypeDef hUsbDeviceFS;
static daisy::DaisySeed hw;
alignas(ReducedTS808) static unsigned char storage[sizeof(ReducedTS808)];
static ReducedTS808* fx;
static float patterns[4][256];
static volatile unsigned blocks, peak, hits, bad, badFrames;
static volatile float sink;
constexpr unsigned totalBlocks=3000;

static void audio(daisy::AudioHandle::InputBuffer, daisy::AudioHandle::OutputBuffer out, size_t frames) {
    const uint32_t begin=DWT->CYCCNT;
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
        fx=new(storage) ReducedTS808;
        uint32_t random=0x504152;
        for(unsigned n=0;n<256;++n) {
            random=random*1664525+1013904223;
            patterns[0][n]=.2f*float(int32_t(random))/2147483648.f;
            patterns[1][n]=(n/17)%2?.9f:-.9f;
            patterns[2][n]=.2f*std::sin(6.28318530718f*7971*n/48000);
            patterns[3][n]=n<128?.8f:-.8f;
        }
        hw.StartAudio(audio);
        while(blocks<totalBlocks) hw.DelayMs(1);
        hw.StopAudio();
    }
    static char msg[384];
    int len=std::snprintf(msg,sizeof(msg),"TS808_PROBE_V1 id=%s seed3=%u core_hz=%lu rate=%u counter=%u blocks=%u bad_frames=%u peak_cycles=%u deadline_hits=%u nonfinite=%u\n",
        PROBE_ID,unsigned(seed3),(unsigned long)hz,rate,unsigned(counter),blocks,badFrames,peak,hits,bad);
    hw.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
    hw.SetLed(true);
    for(;;) {
        uint32_t mask=__get_PRIMASK(); __disable_irq();
        if(len>0 && size_t(len)<sizeof(msg) && hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData &&
           !static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData)->TxState)
            hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(msg),len);
        __set_PRIMASK(mask); hw.DelayMs(750);
    }
}
