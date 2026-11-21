#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#define SLOW_MEMORY DSY_SDRAM_BSS
#include "models.h"
#include "probe_id.h"
#include <cstdio>
extern "C" USBD_HandleTypeDef hUsbDeviceFS;
#include "dfu_control.h"
[[noreturn]] void resourceEnterRomDfu();
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
static constexpr unsigned boundaries[]={0,28,34,36,39};
static constexpr unsigned firstRow=boundaries[WORK_BANK],endRow=boundaries[WORK_BANK+1];
static char report[1600];static size_t reportBytes;
// SPSC byte queue: ISR copies only; foreground drains with a short IRQ guard.
static volatile unsigned rxRead=0,rxWrite=0;
static volatile bool rxUnlocked=false,rxOverflow=false;
static char rx[256];
static void receive(uint8_t* data,uint32_t* length){
 if(!rxUnlocked)return;
 for(uint32_t i=0;i<*length;++i){unsigned next=(rxWrite+1)%sizeof(rx);
  if(next==rxRead){rxOverflow=true;continue;}rx[rxWrite]=char(data[i]);rxWrite=next;}
}
static bool txIdle(){return hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED
 && hUsbDeviceFS.pClassData
 && !static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData)->TxState;}
[[noreturn]] static void fail();
static void controlLoop(){
 resource_control::Protocol protocol(PROBE_ID);
 char reply[400]{};size_t replyBytes=0;
 enum class Tx {none,report,reply,ack};Tx active=Tx::none;
 bool ready=false,hello=false,resetPending=false;uint32_t lastReport=HAL_GetTick()-2000;
 hw.usb_handle.SetReceiveCallback(receive,daisy::UsbHandle::FS_INTERNAL);
 for(;;){
  uint32_t mask=__get_PRIMASK();__disable_irq();
  bool configured=hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED&&hUsbDeviceFS.pClassData;
  if(!configured){
   // A cancelled transfer is not a completed report/ACK. Reconnection starts
   // a fresh report and authorization session; old queued commands expire.
   rxUnlocked=false;rxRead=rxWrite;rxOverflow=false;
   active=Tx::none;ready=false;hello=false;resetPending=false;replyBytes=0;
   protocol=resource_control::Protocol(PROBE_ID);
  }
  bool idle=txIdle();char incoming[64];unsigned n=0;
  bool overflow=rxOverflow;rxOverflow=false;
  while(idle&&!replyBytes&&!resetPending&&rxRead!=rxWrite&&n<sizeof(incoming)){incoming[n++]=rx[rxRead];rxRead=(rxRead+1)%sizeof(rx);}
  if(overflow){rxUnlocked=false;rxRead=rxWrite;}
  __set_PRIMASK(mask);
  if(overflow)fail(); // no reset on truncated/overflowed command streams
  if(idle&&active!=Tx::none){
   if(active==Tx::report){ready=true;protocol.reportCompleted();rxUnlocked=true;}
   if(active==Tx::ack){hw.DelayMs(100);hw.usb_handle.DeInit(daisy::UsbHandle::FS_INTERNAL);hw.DelayMs(100);resourceEnterRomDfu();}
   active=Tx::none;
  }
  // Don't mutate reply until the SDK's asynchronous transfer has completed.
  if(active==Tx::none&&!replyBytes&&!resetPending){
   for(unsigned i=0;i<n;++i){auto event=protocol.feed(incoming[i]);
    if(event==resource_control::Event::hello){
     hello=true;const auto* uid=reinterpret_cast<const uint32_t*>(UID_BASE);
     int count=std::snprintf(reply,sizeof(reply),"PARABLE_CONTROL_V1 artifact_id=%s nonce=%s uid=%08lx%08lx%08lx chip_id=%lu flash_kib=%u board=seed3 handoff=%s\n",PROBE_ID,protocol.nonce(),(unsigned long)uid[0],(unsigned long)uid[1],(unsigned long)uid[2],(unsigned long)DBGMCU->IDCODE,unsigned(*reinterpret_cast<const uint16_t*>(FLASHSIZE_BASE)),resource_control::handoffVersion);
     if(count<=0||size_t(count)>=sizeof(reply))fail();replyBytes=size_t(count);break;
    }else if(event==resource_control::Event::enter){
     int count=std::snprintf(reply,sizeof(reply),"DFU_AUTHORIZED %s\n",protocol.nonce());replyBytes=size_t(count);resetPending=true;break;
    }
   }
  }
  mask=__get_PRIMASK();__disable_irq();
  if(txIdle()&&active==Tx::none){
   if(replyBytes){if(hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(reply),replyBytes)==daisy::UsbHandle::Result::OK){active=resetPending?Tx::ack:Tx::reply;replyBytes=0;}}
   else if(!hello&&(!ready||uint32_t(HAL_GetTick()-lastReport)>=2000)){
    if(hw.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(report),reportBytes)==daisy::UsbHandle::Result::OK){active=Tx::report;lastReport=HAL_GetTick();}
   }
  }
  __set_PRIMASK(mask);hw.SetLed(true);hw.DelayMs(1);
 }
}
[[noreturn]] static void fail(){for(;;){hw.SetLed(true);hw.DelayMs(130);hw.SetLed(false);hw.DelayMs(130);}}
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
        for(unsigned i=firstRow;i<endRow;++i){
            if(!workdown::select(i))continue;
            blocks=peak=hits=bad=wrongFrames=0;
            hw.SetLed((i%2)==0);
            hw.StartAudio(audio);while(blocks<3000&&!hits)hw.DelayMs(1);hw.StopAudio();
            results[i]={blocks,peak,hits,bad,wrongFrames};
        }
    }
    reportBytes=snprintf(report,sizeof(report),"WORK14 id=%s seed3=%u core_hz=%u rate=%u frames=48 counter=%u bank=%u count=%u\n",PROBE_ID,unsigned(seed3),hz,rate,unsigned(counter),WORK_BANK,endRow-firstRow);
    for(unsigned i=firstRow;i<endRow;++i){
        auto v=results[i];
        int n=snprintf(report+reportBytes,sizeof(report)-reportBytes,"ROW %u %u %u %u %u %u\n",i,v.blocks,v.peak,v.hits,v.nonfinite,v.frames);
        if(n<=0||size_t(n)>=sizeof(report)-reportBytes)fail();
        reportBytes+=n;
    }
    hw.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);controlLoop();
}
