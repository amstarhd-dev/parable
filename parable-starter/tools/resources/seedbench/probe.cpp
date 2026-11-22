#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "capricorn_loc.h"
#include "muff_reduced.h"
#include "fits.h"
#include "probe_id.h"
#include <cstdio>
#include <cstring>

extern "C" USBD_HandleTypeDef hUsbDeviceFS;
static daisy::DaisySeed hw;
static constexpr unsigned size=15;
// Four ordered effects (zero bypass), Sustain/Tone/Volume, Loc Drive/fault,
// Centaur and Blender input/output dB with +30 bias, master %, update tag.
struct Patch { uint8_t p[size]; };
static Patch current={{1,2,3,4,75,50,65,65,0,30,32,30,54,20,0}}, pending=current;
static volatile bool changed=false;
static parable::dsp::ReducedMuff muff;
static CapricornLoc loc;
static fitted::Drive klon(fitted::klon6), blender(fitted::blender6);
static float gains[4],master=.2f;
static volatile uint32_t peak=0,hits=0;
static int digit(uint8_t c){
    if(c>='0' && c<='9') return c-'0';
    if(c>='a' && c<='f') return c-'a'+10;
    return -1;
}
static bool valid(const Patch& v){
    unsigned used=0;
    for(unsigned i=0;i<4;++i){
        unsigned id=v.p[i];
        if(id>4 || (id && (used&(1u<<id)))) return false;
        used|=1u<<id;
    }
    for(unsigned i=4;i<=7;++i) if(v.p[i]>100) return false;
    for(unsigned i=9;i<=12;++i) if(v.p[i]>60) return false;
    return v.p[8]<=1 && v.p[13]<=100;
}
static void receive(uint8_t* bytes,uint32_t* count){
    static char frame[size*2]; static unsigned pos=sizeof(frame)+1;
    for(unsigned i=0;i<*count;++i){
        auto c=bytes[i];
        if(c=='@'){pos=0;continue;}
        if(c=='\n' && pos==sizeof(frame)){
            Patch v{};
            for(unsigned j=0;j<size;++j) v.p[j]=16*digit(frame[2*j])+digit(frame[2*j+1]);
            if(valid(v)){
                auto mask=__get_PRIMASK();__disable_irq();pending=v;changed=true;__set_PRIMASK(mask);
            }
        }
        if(pos<sizeof(frame) && digit(c)>=0) frame[pos++]=c; else pos=sizeof(frame)+1;
    }
}
static void configure(){
    auto p=current.p;
    muff.setControls(.01f*p[4],.01f*p[5],.01f*p[6]);
    loc.setRegister(.01f*p[7]);loc.setLeaky(p[8]);
    for(unsigned j=0;j<4;++j) gains[j]=std::pow(10.f,(int(p[9+j])-30)*.05f);
}
static void play(daisy::AudioHandle::InputBuffer in,daisy::AudioHandle::OutputBuffer out,size_t n){
    auto start=DWT->CYCCNT;
    if(changed){
        auto mask=__get_PRIMASK();__disable_irq();current=pending;changed=false;__set_PRIMASK(mask);
        configure();
    }
    for(unsigned i=0;i<n;++i){
        float x=in[0][i];
        for(unsigned slot=0;slot<4;++slot){
            switch(current.p[slot]){
                case 1:x=muff.process(x)*3.98107f;break;
                case 2:x=loc.process(x);break;
                case 3:x=klon.process(std::clamp(x*gains[0],-1.f,1.f))*gains[1];break;
                case 4:x=blender.process(std::clamp(x*gains[2],-1.f,1.f))*gains[3];break;
            }
            x=std::clamp(x,-1.f,1.f);
        }
        master+=.003f*(.01f*current.p[13]-master);
        out[0][i]=out[1][i]=std::isfinite(x)?x*master:0.f;
    }
    auto elapsed=DWT->CYCCNT-start;
    if(elapsed>peak)peak=elapsed;
    if(elapsed>=480000)++hits;
}
int main(){
    hw.Init(true); hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48); SystemCoreClockUpdate();
    if(hw.CheckBoardVersion()!=daisy::DaisySeed::BoardVersion::DAISY_SEED_3 || SystemCoreClock!=480000000) for(;;){}
    CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk; DWT->LAR=0xC5ACCE55; DWT->CYCCNT=0; DWT->CTRL|=1;
    configure();hw.StartAudio(play);hw.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
    hw.usb_handle.SetReceiveCallback(receive,daisy::UsbHandle::FS_INTERNAL);hw.SetLed(true);
    static char tx[180];
    for(;;){
        auto mask=__get_PRIMASK();__disable_irq();Patch snapshot=current;auto cycles=peak, misses=hits;__set_PRIMASK(mask);
        char hex[2*size+1]; for(unsigned j=0;j<size;++j)std::snprintf(hex+2*j,3,"%02x",unsigned(snapshot.p[j]));
        if(hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData &&
           !static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData)->TxState){
            int n=std::snprintf(tx,sizeof(tx),"PARABLE4 %s %s %lu %lu\n",PROBE_ID,hex,(unsigned long)cycles,(unsigned long)misses);
            hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(tx),n);
        }
        hw.DelayMs(120);
    }
}
