// Experimental Seed3-only RAM identity instrument; never linked into firmware.
#include "stm32h750xx.h"
#include "identity_config.h"
#include "protocol.h"
extern "C" [[noreturn]] void ram_identity_main();
extern "C" __attribute__((naked,used,noreturn)) void Ram_Entry() {
 asm volatile("cpsid i\n ldr r0, =0x20020000\n msr msp, r0\n"
              "movs r0, #0\n msr control, r0\n isb\n b ram_identity_main");
}
extern "C" [[noreturn]] void Ram_Fault() { __disable_irq(); for(;;) __NOP(); }
using Handler = void (*)();
// Explicit complete table, including all 150 H750 external IRQ slots.
__attribute__((section(".vectors"),used)) const Handler vectors[] = {
 reinterpret_cast<Handler>(0x20020000), Ram_Entry,
 VECTOR_FAULT_ENTRIES
};
extern "C" [[noreturn]] void ram_identity_main() {
 // No constructors, HAL, USB initialization or interrupt enable before reset.
 const uint32_t entryCcr=SCB->CCR, entryVtor=SCB->VTOR;
 SysTick->CTRL=0; SysTick->LOAD=0; SysTick->VAL=0;
 for(unsigned i=0;i<(unsigned(WAKEUP_PIN_IRQn)+32)/32;++i) {
  NVIC->ICER[i]=0xffffffff; NVIC->ICPR[i]=0xffffffff;
 }
 SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk|SCB_ICSR_PENDSVCLR_Msk;
 __DSB(); __ISB();
 if(SCB->CCR&SCB_CCR_DC_Msk) SCB_DisableDCache();
 SCB_DisableICache();
 SCB->VTOR=0x24040000; __DSB(); __ISB();
 // Reset ROM USB peripherals; never service an inherited USB IRQ.
 RCC->AHB1RSTR|=RCC_AHB1RSTR_USB1OTGHSRST|RCC_AHB1RSTR_USB2OTGFSRST;
 __DSB();
 RCC->AHB1RSTR&=~(RCC_AHB1RSTR_USB1OTGHSRST|RCC_AHB1RSTR_USB2OTGFSRST);
 auto* m=reinterpret_cast<volatile uint32_t*>(0x24041000);
 constexpr uint32_t identity[8]=IDENTITY_WORDS;
 if(!ram_identity::validRequest(m,identity)) Ram_Fault();
 const auto* uid=reinterpret_cast<const volatile uint32_t*>(UID_BASE);
 m[16]=uid[0]; m[17]=uid[1]; m[18]=uid[2];
 m[19]=DBGMCU->IDCODE;
 m[20]=*reinterpret_cast<const volatile uint16_t*>(FLASHSIZE_BASE);
 m[21]=RCC->RSR; m[22]=entryCcr; m[23]=entryVtor;
 m[24]=SCB->VTOR; m[25]=3; m[26]=__get_IPSR(); m[27]=__get_CONTROL(); m[28]=__get_PRIMASK(); // only PG3 is driven, as in pinned libDaisy STM mode
 m[31]=ram_identity::checksum(m); __DSB();
 m[0]=0x50494452; __DSB(); // commit last, while cache is disabled
 RCC->AHB4ENR|=RCC_AHB4ENR_GPIOGEN; (void)RCC->AHB4ENR; __DSB();
 GPIOG->BSRR=1u<<3;
 GPIOG->OTYPER&=~(1u<<3);
 GPIOG->PUPDR&=~(3u<<6);
 GPIOG->OSPEEDR&=~(3u<<6);
 GPIOG->MODER=(GPIOG->MODER&~(3u<<6))|(1u<<6);
 // At <=480MHz even one cycle/iteration exceeds SDK's 10ms charge delay.
 for(uint32_t n=0;n<5000000;++n) __NOP();
 __DSB(); NVIC_SystemReset();
 for(;;) __NOP();
}
