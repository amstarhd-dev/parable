#include "stm32h750xx.h"
#include "diag_config.h"
#include "../ram_identity/protocol.h"
extern "C" [[noreturn]] void Diag_Entry();
extern "C" [[noreturn]] void Diag_Fault();
using Handler=void(*)();
__attribute__((section(".vectors"),used)) const Handler vectors[]={
 reinterpret_cast<Handler>(0x2001ffc0),Diag_Entry,VECTOR_FAULT_ENTRIES
};
static inline void mark(unsigned stage) {
 auto* p=reinterpret_cast<volatile uint32_t*>(0x24042040+(stage-1)*32);
 p[0]=0x53544745;p[1]=stage;p[2]=~uint32_t(stage);
 p[3]=__get_IPSR();p[4]=__get_MSP();p[5]=__get_CONTROL();p[6]=__get_PRIMASK();
 p[7]=0x434d4954;
 __DSB();SCB->DCCMVAC=uint32_t(p);__DSB();__ISB();
}
extern "C" [[noreturn]] void Diag_Main() {
 mark(6);
 const uint32_t ccr=SCB->CCR;
 SysTick->CTRL=0;SysTick->LOAD=0;SysTick->VAL=0;
 for(unsigned i=0;i<(unsigned(WAKEUP_PIN_IRQn)+32)/32;++i){NVIC->ICER[i]=0xffffffff;NVIC->ICPR[i]=0xffffffff;}
 SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk|SCB_ICSR_PENDSVCLR_Msk;__DSB();__ISB();
 mark(7);if(SCB->CCR&SCB_CCR_DC_Msk)SCB_DisableDCache();
 mark(8);SCB_DisableICache();
 mark(9);RCC->AHB1RSTR|=RCC_AHB1RSTR_USB1OTGHSRST|RCC_AHB1RSTR_USB2OTGFSRST;__DSB();
 RCC->AHB1RSTR&=~(RCC_AHB1RSTR_USB1OTGHSRST|RCC_AHB1RSTR_USB2OTGFSRST);
 mark(10);
 auto* m=reinterpret_cast<volatile uint32_t*>(0x24041000);
 constexpr uint32_t identity[8]=IDENTITY_WORDS;
 if(!ram_identity::validRequest(m,identity))Diag_Fault();
 auto* uid=reinterpret_cast<const volatile uint32_t*>(UID_BASE);
 mark(11);m[16]=uid[0];mark(12);m[17]=uid[1];mark(13);m[18]=uid[2];
 mark(14);m[19]=DBGMCU->IDCODE;
 mark(15);m[20]=*reinterpret_cast<const volatile uint16_t*>(FLASHSIZE_BASE);
 mark(16);m[21]=RCC->RSR;m[22]=ccr;m[23]=0x24040000;m[24]=SCB->VTOR;
 m[25]=DIAG_MODE==2?3:0;m[26]=__get_IPSR();m[27]=__get_CONTROL();m[28]=__get_PRIMASK();
 m[31]=ram_identity::checksum(m);__DSB();m[0]=0x50494452;__DSB();
 mark(17);
#if DIAG_MODE == 2
 mark(18);RCC->AHB4ENR|=RCC_AHB4ENR_GPIOGEN;(void)RCC->AHB4ENR;__DSB();
 mark(19);GPIOG->BSRR=1u<<3;GPIOG->OTYPER&=~(1u<<3);GPIOG->PUPDR&=~(3u<<6);GPIOG->OSPEEDR&=~(3u<<6);GPIOG->MODER=(GPIOG->MODER&~(3u<<6))|(1u<<6);
 mark(20);for(uint32_t n=0;n<5000000;++n)__NOP();
 mark(21);NVIC_SystemReset();
#endif
 for(;;)__NOP();
}
