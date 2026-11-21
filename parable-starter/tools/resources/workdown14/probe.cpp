#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#define SLOW_MEMORY DSY_SDRAM_BSS
#include "models.h"
#include "probe_id.h"
#include <cstdio>
extern "C" USBD_HandleTypeDef hUsbDeviceFS;
using namespace workdown;
static daisy::DaisySeed hw;
struct Result {unsigned blocks,peak,hits,nonfinite,frames;};
static Result results[modelCount];
static volatile unsigned blocks,peak,hits,bad,wrongFrames;
static volatile float sink;
static float patterns[4][256];
static void audio(daisy::AudioHandle::InputBuffer,daisy::AudioHandle::OutputBuffer out,size_t n){
    const unsigned begin=DWT->CYCCNT,block=blocks;
    const bool capture=block<3000&&!hits;
    for(size_t j=0;j<n;++j){
        if(capture){
            float y=process(patterns[(block/32)%4][(block*48+j)%256]);
            sink=y;if(!std::isfinite(y))++bad;
        }
        out[0][j]=out[1][j]=0;
    }
    __DSB();const unsigned elapsed=DWT->CYCCNT-begin;
    if(capture){
        if(n!=48)++wrongFrames;
        if(elapsed>peak)peak=elapsed;
        if(elapsed>=480000)++hits;
        blocks=block+1;
    }
}
int main(){
    hw.Init(true);hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);SystemCoreClockUpdate();
    const unsigned hz=SystemCoreClock,rate=unsigned(hw.AudioSampleRate());
    const bool seed3=hw.CheckBoardVersion()==daisy::DaisySeed::BoardVersion::DAISY_SEED_3;
    CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR=0xC5ACCE55;DWT->CYCCNT=0;DWT->CTRL|=DWT_CTRL_CYCCNTENA_Msk;
    __DSB();__ISB();const unsigned first=DWT->CYCCNT;
    for(unsigned i=0;i<32;++i)__NOP();
    const bool counter=DWT->CYCCNT!=first;
    if(seed3 && hz==480000000 && rate==48000 && counter){
        uint32_t random=0x504152;
        for(unsigned i=0;i<256;++i){
            random=random*1664525+1013904223;
            patterns[0][i]=.2f*float(int32_t(random))/2147483648.f;
            patterns[1][i]=(i/17)%2?.9f:-.9f;
            patterns[2][i]=.2f*std::sin(6.28318530718f*7971*i/48000);
            patterns[3][i]=i<128?.8f:-.8f;
        }
        for(unsigned i=0;i<modelCount;++i){
            if(!workdown::select(i))continue;
            blocks=peak=hits=bad=wrongFrames=0;
            hw.SetLed((i%2)==0);
            hw.StartAudio(audio);while(blocks<3000&&!hits)hw.DelayMs(1);hw.StopAudio();
            results[i]={blocks,peak,hits,bad,wrongFrames};
        }
    }
    static char lines[modelCount+1][180];
    snprintf(lines[0],180,"WORK14 id=%s seed3=%u core_hz=%u rate=%u frames=48 counter=%u count=%u\n",
             PROBE_ID,unsigned(seed3),hz,rate,unsigned(counter),modelCount);
    for(unsigned i=0;i<modelCount;++i){
        const auto& v=results[i];snprintf(lines[i+1],180,"ROW %u %u %u %u %u %u\n",i,v.blocks,v.peak,v.hits,v.nonfinite,v.frames);
    }
    hw.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);hw.SetLed(true);unsigned row=0;
    for(;;){
        uint32_t mask=__get_PRIMASK();__disable_irq();
        if(hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData &&
           !static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData)->TxState){
            hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(lines[row]),strlen(lines[row]));
            row=(row+1)%(modelCount+1);
        }
        __set_PRIMASK(mask);hw.DelayMs(35);
    }
}
