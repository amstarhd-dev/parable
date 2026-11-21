// Experimental benchmark-only STM32H750 ROM handoff. See RUNNER.md / Gate 0.
#include "stm32h7xx.h"
#include "sys/system.h"
#include <cstdint>

namespace {
constexpr uint32_t cookie = 0x50444631, rom = 0x1ff09800;
__attribute__((section(".parable_handoff"), aligned(32), used))
volatile uint32_t mailbox[8];
}
extern "C" void __real_SystemInit();
extern "C" void __wrap_SystemInit() {
    // Pinned Reset_Handler has copied .data/zeroed .bss, but has NOT called
    // SystemInit, constructors, HAL, clocks, caches, audio or USB.
    const uint32_t reason = RCC->RSR;
    const bool requested = mailbox[0] == cookie && mailbox[1] == ~cookie
                           && mailbox[2] == 1 && mailbox[3] == ~uint32_t(1);
    mailbox[0] = 0; // consume before inspecting ROM; never loop into it
    __DSB();
    if(requested && (reason & RCC_RSR_SFTRSTF)
       && !(reason & (RCC_RSR_PORRSTF | RCC_RSR_BORRSTF))) {
        const auto* vectors = reinterpret_cast<const uint32_t*>(rom);
        const uint32_t sp = vectors[0], entry = vectors[1];
        const bool stack = (sp > 0x20000000 && sp <= 0x20020000)
                           || (sp > 0x24000000 && sp <= 0x24080000);
        if(stack && !(sp & 7) && (entry & 1)
           && entry >= 0x1ff00000 && entry < 0x1ff20000) {
            __disable_irq();
            SCB->VTOR = rom;
            __DSB(); __ISB();
            // No C++ epilogue or stack access after replacing MSP.
            asm volatile("msr msp, %0\n movs r2, #0\n msr control, r2\n"
                         "isb\n cpsie i\n bx %1"
                         : : "r"(sp), "r"(entry) : "r2", "memory");
            __builtin_unreachable();
        }
    }
    __real_SystemInit(); // ordinary boot/recovery unchanged
}
[[noreturn]] void resourceEnterRomDfu() {
    __disable_irq();
    RCC->RSR |= RCC_RSR_RMVF; // subsequent reset must be software-only
    mailbox[0] = cookie; mailbox[1] = ~cookie;
    mailbox[2] = 1; mailbox[3] = ~uint32_t(1);
    if(SCB->CCR & SCB_CCR_DC_Msk)
        SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(
            const_cast<uint32_t*>(mailbox)), sizeof(mailbox));
    __DSB(); __ISB();
    // Prefer libDaisy's board-specific BOOT-pin path instead of a naked reset.
    // It drives PG3 high, waits 10 ms for the BOOT capacitor, then resets.
    // Leave interrupts enabled here: its HAL_Delay() depends on the HAL tick.
    // If BOOT sampling selects ROM, the early SystemInit wrapper is unused.
    // If hardware instead selects application flash, the pre-existing cookie
    // lets the wrapped SystemInit attempt its ROM-vector handoff as fallback.
    daisy::System::ResetToBootloader(daisy::System::BootloaderMode::STM);
    // Defensive only: the SDK reset call should never return.
    NVIC_SystemReset();
    for(;;) {}
}
