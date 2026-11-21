// Experimental benchmark executable only. Never linked into production firmware.
#include "daisy_seed.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "runtime_measurement.h"
#include "resource_config.h"
#include "adapters.h"
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
__attribute__((noinline)) static void kernel48(const float* in,float* out){for(unsigned n=0;n<48;++n){float v=model->process(in[n]);out[n]=v;if(!std::isfinite(v))++finiteFailures;}}
static void observe(const float* in,float* out,size_t frames){
 uint32_t begin=counter?DWT->CYCCNT:0;
 if(frames==48)kernel48(in,out);
 __DSB();uint32_t end=counter?DWT->CYCCNT:0;capture.observe(begin,end,uint32_t(frames));
}
static void audio(daisy::AudioHandle::InputBuffer in,daisy::AudioHandle::OutputBuffer out,size_t frames){
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
 const auto* s=capture.snapshot();if(!s)fail();
 int count=std::snprintf(report,sizeof(report),"PARABLE_RESOURCE_V1 artifact_id=%s profile=%s execution=%s stimulus=synthetic_mix mode=%u route=%u\nsource_set_sha256=%s\ncore_hz_rcc=%lu sample_rate=48000 block_frames=48 deadline_cycles=%lu counter_enabled=%u\nblocks=%lu valid_blocks=%lu max_cycles=%lu deadline_hits=%lu invalid_intervals=%lu frame_mismatches=%lu\nreserved_sdram_bytes=%lu used_sdram_bytes=%lu finite_output_failures=%lu\nfpscr=%lu fpdscr=%lu icache_enabled=%u dcache_enabled=%u\nEND_RESOURCE_CAPTURE\n",BENCH_ARTIFACT_ID,BENCH_PROFILE,BENCH_EXECUTION,unsigned(BENCH_MODE),unsigned(BENCH_ROUTE_VALUE),BENCH_SOURCE_SHA,(unsigned long)budget.coreHz,(unsigned long)budget.deadlineCycles,unsigned(counter),(unsigned long)s->blocks,(unsigned long)s->validBlocks,(unsigned long)s->maxCycles,(unsigned long)s->atOrAboveDeadline,(unsigned long)s->invalidIntervals,(unsigned long)s->frameMismatches,(unsigned long)BENCH_RESERVED_SDRAM_BYTES,(unsigned long)(storageUsed*4+BENCH_MODEL_SDRAM_BYTES),(unsigned long)finiteFailures,(unsigned long)__get_FPSCR(),(unsigned long)FPU->FPDSCR,unsigned((SCB->CCR&SCB_CCR_IC_Msk)!=0),unsigned((SCB->CCR&SCB_CCR_DC_Msk)!=0));
 if(count<=0||size_t(count)>=sizeof(report))fail();reportBytes=size_t(count);hardware.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
 for(;;){uint32_t mask=__get_PRIMASK();__disable_irq();if(hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED&&hUsbDeviceFS.pClassData){auto* cdc=static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData);if(!cdc->TxState)hardware.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(report),reportBytes);}__set_PRIMASK(mask);hardware.SetLed(true);hardware.DelayMs(2000);}
}
