#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "wire.h"
#include "probe_id.h"
#include <cstdio>
extern "C" USBD_HandleTypeDef hUsbDeviceFS;
using namespace graph;
static daisy::DaisySeed hw;
static Bulk DSY_SDRAM_BSS bulk[4][2];
// Large nonlinear lookup state also lives in SDRAM for polyphony.
alignas(Unit) static uint8_t DSY_SDRAM_BSS unitStorage[4][2][sizeof(Unit)];
static Unit* units[4][2];
struct Scene {Voice voice[4];Patch patch;};
static Scene scenes[2];
static volatile unsigned active=0;
static volatile int staged=-1;
static Patch incoming;
static volatile bool waiting=false;
static volatile uint32_t peak=0,hits=0;
static float master=0;
static void receive(uint8_t* data,uint32_t* count){
    static unsigned pos=2*bytes+1;static Wire raw;
    for(unsigned i=0;i<*count;++i){
        uint8_t c=data[i];
        if(c=='@'){pos=0;continue;}
        if(c=='\n' && pos==2*bytes){Patch p{};if(decode(raw,p)){incoming=p;__DMB();waiting=true;}}
        unsigned n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:16;
        if(pos>=2*bytes||n>=16){pos=2*bytes+1;continue;}
        if(pos%2)raw[pos/2]|=n;else raw[pos/2]=n<<4;
        ++pos;
    }
}
static bool prepare(const Patch& p){
    const unsigned old=active, next=1-old;
    const Scene& prev=scenes[old];Scene& dest=scenes[next];
    for(unsigned i=0;i<4;++i){
        auto& s=p.slots[i];auto& v=dest.voice[i];
        const Voice* before=nullptr;
        for(auto& candidate:prev.voice)if(candidate.unit && candidate.config.key==s.key)before=&candidate;
        bool rebuild=!before || before->config.effect!=s.effect ||
            (s.effect==Muff && s.p[4]!=before->config.p[4]) ||
            (s.effect==Scorpio && s.p[2]!=before->config.p[2]);
        v.config=s;
        if(rebuild){
            unsigned bank=before && before->unit==units[s.key][0];
            v.unit=units[s.key][bank];if(!v.unit->prepare(s))return false;
        }else v.unit=before->unit;
        v.coefficients();
    }
    dest.patch=p;__DMB();staged=next;return true;
}
static void audio(daisy::AudioHandle::InputBuffer in,daisy::AudioHandle::OutputBuffer out,size_t n){
    uint32_t begin=DWT->CYCCNT;
    if(staged>=0){active=staged;for(auto& v:scenes[active].voice)v.apply();__DMB();staged=-1;}
    auto& scene=scenes[active];float samples[48];if(n!=48){for(unsigned j=0;j<n;++j)out[0][j]=out[1][j]=0;return;}
    std::copy_n(in[0],n,samples);
    for(auto& v:scene.voice)if(v.unit && !v.config.bypass && v.config.effect)
        for(auto& sample:samples)sample=v.sample(bound(sample));
    for(unsigned j=0;j<n;++j){master+=.003f*(.01f*scene.patch.volume-master);
        out[0][j]=out[1][j]=std::isfinite(samples[j])?bound(samples[j])*master:0;}
    auto elapsed=DWT->CYCCNT-begin;if(elapsed>peak)peak=elapsed;if(elapsed>=480000)++hits;
}
int main(){
    hw.Init(true);hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);hw.SetAudioBlockSize(48);
    SystemCoreClockUpdate();if(hw.CheckBoardVersion()!=daisy::DaisySeed::BoardVersion::DAISY_SEED_3 || SystemCoreClock!=480000000)for(;;){}
    for(unsigned i=0;i<4;++i)for(unsigned b=0;b<2;++b){units[i][b]=new(unitStorage[i][b]) Unit;units[i][b]->slow=&bulk[i][b];}
    Patch start={{{0,Muff,0,{75,50,65,42,0,10}},{1,Tape,0,{310,50,40,4500,6000,24,0}},{2,Spring,0,{100,30,100}},{3,Rotary,0,{35,42,0}}},20,0};
    if(!prepare(start))for(;;){}
    CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk;DWT->LAR=0xC5ACCE55;DWT->CYCCNT=0;DWT->CTRL|=1;
    hw.StartAudio(audio);hw.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);hw.usb_handle.SetReceiveCallback(receive,daisy::UsbHandle::FS_INTERNAL);hw.SetLed(true);
    static char tx[300];unsigned previous=0;
    for(;;){
        if(waiting && staged<0){
            auto mask=__get_PRIMASK();__disable_irq();Patch p=incoming;waiting=false;__set_PRIMASK(mask);
            prepare(p);
        }
        auto now=HAL_GetTick();
        if(now-previous>=100 && hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData && !static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData)->TxState){
            auto wire=encode(scenes[active].patch);char hex[2*bytes+1];const char digits[]="0123456789abcdef";
            for(unsigned i=0;i<bytes;++i){hex[2*i]=digits[wire[i]>>4];hex[2*i+1]=digits[wire[i]&15];}
            hex[2*bytes]=0;int len=std::snprintf(tx,sizeof(tx),"PGRAPH1 %s %s %lu %lu\n",PROBE_ID,hex,(unsigned long)peak,(unsigned long)hits);
            hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(tx),len);previous=now;
        }
        hw.DelayMs(1);
    }
}
