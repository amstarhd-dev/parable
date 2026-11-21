// Experimental benchmark executable only. Never linked into production firmware.
#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "runtime_measurement.h"
#include "resource_config.h"
#include "adapters.h"
#ifdef BENCH_USB_CONTROL
#include "dfu_control.h"
[[noreturn]] void resourceEnterRomDfu();
#endif
#include <cmath>
#include <cstdio>
#include <new>
extern "C" USBD_HandleTypeDef hUsbDeviceFS;
static daisy::DaisySeed hardware;
namespace resource_bench {using Selected=BENCH_SELECTED;}
using resource_bench::Selected;
#ifdef BENCH_MODEL_SDRAM
alignas(Selected) static unsigned char DSY_SDRAM_BSS modelBytes[sizeof(Selected)];
#else
alignas(Selected) static unsigned char modelBytes[sizeof(Selected)];
#endif
static Selected* model=nullptr;
#if BENCH_ARENA_SAMPLES > 0
static float DSY_SDRAM_BSS storage[BENCH_ARENA_SAMPLES];
#endif
static size_t storageUsed=0;
namespace resource_bench {
float* reserveSamples(size_t n) noexcept {
#if BENCH_ARENA_SAMPLES > 0
 size_t start=(storageUsed+7)&~size_t(7);if(start+n>BENCH_ARENA_SAMPLES)return nullptr;storageUsed=start+n;return storage+start;
#else
 (void)n;return nullptr;
#endif
}
}
static float patterns[4][256],output[48];
static parable::first_boot::Capture capture;
static parable::first_boot::Budget budget;
static bool counter=false;
static uint32_t blockIndex=0,finiteFailures=0;
static volatile float sink=0;
static char report[1600];static size_t reportBytes=0;
#ifdef BENCH_USB_CONTROL
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
 resource_control::Protocol protocol(BENCH_ARTIFACT_ID);
 char reply[400]{};size_t replyBytes=0;
 enum class Tx {none,report,reply,ack};Tx active=Tx::none;
 bool ready=false,hello=false,resetPending=false;uint32_t lastReport=HAL_GetTick()-2000;
 hardware.usb_handle.SetReceiveCallback(receive,daisy::UsbHandle::FS_INTERNAL);
 for(;;){
  uint32_t mask=__get_PRIMASK();__disable_irq();
  bool configured=hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED&&hUsbDeviceFS.pClassData;
  if(!configured){
   // A cancelled transfer is not a completed report/ACK. Reconnection starts
   // a fresh report and authorization session; old queued commands expire.
   rxUnlocked=false;rxRead=rxWrite;rxOverflow=false;
   active=Tx::none;ready=false;hello=false;resetPending=false;replyBytes=0;
   protocol=resource_control::Protocol(BENCH_ARTIFACT_ID);
  }
  bool idle=txIdle();char incoming[64];unsigned n=0;
  bool overflow=rxOverflow;rxOverflow=false;
  while(idle&&!replyBytes&&!resetPending&&rxRead!=rxWrite&&n<sizeof(incoming)){incoming[n++]=rx[rxRead];rxRead=(rxRead+1)%sizeof(rx);}
  if(overflow){rxUnlocked=false;rxRead=rxWrite;}
  __set_PRIMASK(mask);
  if(overflow)fail(); // no reset on truncated/overflowed command streams
  if(idle&&active!=Tx::none){
   if(active==Tx::report){ready=true;protocol.reportCompleted();rxUnlocked=true;}
   if(active==Tx::ack){hardware.DelayMs(100);hardware.usb_handle.DeInit(daisy::UsbHandle::FS_INTERNAL);hardware.DelayMs(100);resourceEnterRomDfu();}
   active=Tx::none;
  }
  // Don't mutate reply until the SDK's asynchronous transfer has completed.
  if(active==Tx::none&&!replyBytes&&!resetPending){
   for(unsigned i=0;i<n;++i){auto event=protocol.feed(incoming[i]);
    if(event==resource_control::Event::hello){
     hello=true;const auto* uid=reinterpret_cast<const uint32_t*>(UID_BASE);
     int count=std::snprintf(reply,sizeof(reply),"PARABLE_CONTROL_V1 artifact_id=%s nonce=%s uid=%08lx%08lx%08lx chip_id=%lu flash_kib=%u board=seed3 handoff=%s\n",BENCH_ARTIFACT_ID,protocol.nonce(),(unsigned long)uid[0],(unsigned long)uid[1],(unsigned long)uid[2],(unsigned long)DBGMCU->IDCODE,unsigned(*reinterpret_cast<const uint16_t*>(FLASHSIZE_BASE)),resource_control::handoffVersion);
     if(count<=0||size_t(count)>=sizeof(reply))fail();replyBytes=size_t(count);break;
    }else if(event==resource_control::Event::enter){
     int count=std::snprintf(reply,sizeof(reply),"DFU_AUTHORIZED %s\n",protocol.nonce());replyBytes=size_t(count);resetPending=true;break;
    }
   }
  }
  mask=__get_PRIMASK();__disable_irq();
  if(txIdle()&&active==Tx::none){
   if(replyBytes){if(hardware.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(reply),replyBytes)==daisy::UsbHandle::Result::OK){active=resetPending?Tx::ack:Tx::reply;replyBytes=0;}}
   else if(!hello&&(!ready||uint32_t(HAL_GetTick()-lastReport)>=2000)){
    if(hardware.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(report),reportBytes)==daisy::UsbHandle::Result::OK){active=Tx::report;lastReport=HAL_GetTick();}
   }
  }
  __set_PRIMASK(mask);hardware.SetLed(true);hardware.DelayMs(1);
 }
}
#endif
__attribute__((noinline)) static void kernel48(const float* in,float* out){for(unsigned n=0;n<48;++n){float v=model->process(in[n]);out[n]=v;if(!std::isfinite(v))++finiteFailures;}}
static void observe(const float* in,float* out,size_t frames){
 uint32_t begin=counter?DWT->CYCCNT:0;
 if(frames==48)kernel48(in,out);
 __DSB();uint32_t end=counter?DWT->CYCCNT:0;capture.observe(begin,end,uint32_t(frames));
}
static void audio(daisy::AudioHandle::InputBuffer in,daisy::AudioHandle::OutputBuffer out,size_t frames){
 if(!capture.recording()){for(size_t n=0;n<frames;++n)out[0][n]=out[1][n]=0;return;}
 uint32_t begin=counter?DWT->CYCCNT:0;(void)in;float input[48];for(unsigned n=0;n<48;++n)input[n]=patterns[(blockIndex/32)%4][(blockIndex*48+n)%256];
 if(frames==48)kernel48(input,output);for(size_t n=0;n<frames;++n)out[0][n]=out[1][n]=n<48?output[n]:0;__DSB();uint32_t end=counter?DWT->CYCCNT:0;capture.observe(begin,end,uint32_t(frames));++blockIndex;
}
[[noreturn]] static void fail(){for(;;){hardware.SetLed(true);hardware.DelayMs(100);hardware.SetLed(false);hardware.DelayMs(100);}}
int main(){hardware.Configure();hardware.Init();
 if(hardware.CheckBoardVersion()!=daisy::DaisySeed::BoardVersion::DAISY_SEED_3)fail();
 hardware.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);hardware.SetAudioBlockSize(48);
 model=new(modelBytes) Selected;
#ifdef BENCH_ROUTE
 model->route=BENCH_ROUTE_VALUE;
#endif
#ifdef BENCH_UTILITY_KIND
 model->kind=BENCH_UTILITY_KIND;
#endif
 if(!model->prepare(48000,BENCH_MODE))fail();
 uint32_t random=0x504152;for(unsigned n=0;n<256;++n){random=random*1664525+1013904223;patterns[0][n]=.2f*float(int32_t(random))/2147483648.f;patterns[1][n]=(n/17)%2?.9f:-.9f;patterns[2][n]=.2f*std::sin(6.28318530718f*7971*n/48000);patterns[3][n]=n<128?.8f:-.8f;}
 SystemCoreClockUpdate();if(!parable::first_boot::makeBudget(SystemCoreClock,hardware.AudioSampleRate(),48,budget))fail();
 CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk;DWT->LAR=0xC5ACCE55;DWT->CYCCNT=0;DWT->CTRL|=DWT_CTRL_CYCCNTENA_Msk;__DSB();__ISB();uint32_t first=DWT->CYCCNT;for(unsigned n=0;n<32;++n)__NOP();counter=(DWT->CTRL&DWT_CTRL_CYCCNTENA_Msk)&&DWT->CYCCNT!=first;
 if(!capture.initialize(budget,counter,BENCH_CAPTURE_BLOCKS))fail();
#ifdef BENCH_AUDIO_CALLBACK
 hardware.StartAudio(audio);while(!capture.snapshot())hardware.DelayMs(1);hardware.StopAudio();
#else
 while(capture.recording()){float input[48];for(unsigned n=0;n<48;++n)input[n]=patterns[(blockIndex/32)%4][(blockIndex*48+n)%256];observe(input,output,48);sink=output[47];++blockIndex;if(blockIndex%32==0)hardware.SetLed((blockIndex/32)%2);}
#endif
 const auto* s=capture.snapshot();if(!s)fail();const auto stats=model->counters();
 auto word=[](uint64_t x){return static_cast<unsigned long>(x>0xffffffffULL?0xffffffffULL:x);};
 int count=std::snprintf(report,sizeof(report),"PARABLE_RESOURCE_V1 artifact_id=%s profile=%s execution=%s stimulus=synthetic_mix mode=%u route=%u\nsource_set_sha256=%s\ncore_hz_rcc=%lu sample_rate=48000 block_frames=48 deadline_cycles=%lu counter_enabled=%u\nblocks=%lu valid_blocks=%lu max_cycles=%lu deadline_hits=%lu invalid_intervals=%lu frame_mismatches=%lu\nreserved_sdram_bytes=%lu used_sdram_bytes=%lu finite_output_failures=%lu\nfpscr=%lu fpdscr=%lu icache_enabled=%u dcache_enabled=%u\nnative_failures=%lu native_evaluations=%lu native_solves=%lu native_max_iterations=%lu native_containment=%lu\nEND_RESOURCE_CAPTURE\n",BENCH_ARTIFACT_ID,BENCH_PROFILE,BENCH_EXECUTION,unsigned(BENCH_MODE),unsigned(BENCH_ROUTE_VALUE),BENCH_SOURCE_SHA,(unsigned long)budget.coreHz,(unsigned long)budget.deadlineCycles,unsigned(counter),(unsigned long)s->blocks,(unsigned long)s->validBlocks,(unsigned long)s->maxCycles,(unsigned long)s->atOrAboveDeadline,(unsigned long)s->invalidIntervals,(unsigned long)s->frameMismatches,(unsigned long)BENCH_RESERVED_SDRAM_BYTES,(unsigned long)(storageUsed*4+BENCH_MODEL_SDRAM_BYTES),(unsigned long)finiteFailures,(unsigned long)__get_FPSCR(),(unsigned long)FPU->FPDSCR,unsigned((SCB->CCR&SCB_CCR_IC_Msk)!=0),unsigned((SCB->CCR&SCB_CCR_DC_Msk)!=0),word(stats.failures),word(stats.evaluations),word(stats.solves),word(stats.max_iterations),word(stats.containment));
 if(count<=0||size_t(count)>=sizeof(report))fail();reportBytes=size_t(count);hardware.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
#ifdef BENCH_USB_CONTROL
 controlLoop();
#else
 for(;;){uint32_t mask=__get_PRIMASK();__disable_irq();if(hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED&&hUsbDeviceFS.pClassData){auto* cdc=static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData);if(!cdc->TxState)hardware.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(report),reportBytes);}__set_PRIMASK(mask);hardware.SetLed(true);hardware.DelayMs(2000);}
#endif
}
