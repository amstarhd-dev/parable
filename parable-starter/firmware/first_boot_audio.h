#pragma once
// Included only in the two FIRST_BOOT audio builds, after hardware/audioEngine.
// Reuses pinned CMSIS, libDaisy USB CDC and the existing audio processing loop.
#include "runtime_measurement.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include <cstdio>

extern "C" USBD_HandleTypeDef hUsbDeviceFS;
namespace first_boot_runtime {
using namespace parable::first_boot;
static Budget budget;
static Capture capture;
static bool counterAvailable = false;
static bool audioAttempted = false; // Control side only; includes partial-start cleanup.
static uint32_t loadCycles = 0, loadMs = 0;
static bool loadCyclesValid = false;
static char report[1024]; // Formatted ONCE, immutable for all asynchronous sends.
static size_t reportBytes = 0;

[[noreturn]] inline void fail(LedStatus status) {
    if (audioAttempted) hardware.StopAudio();
    while (true) {
        hardware.SetLed(statusLed(status, daisy::System::GetNow()));
        daisy::System::Delay(10);
    }
}
inline bool enableCounter() noexcept {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55; // ARM debug unlock; audited pinned CMSIS/Daisy precedent.
    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) return false;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB(); __ISB();
    const uint32_t before = DWT->CYCCNT;
    for (unsigned i = 0; i < 32; ++i) __NOP();
    return (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) && DWT->CYCCNT != before;
}
inline void initialize(float rate) {
    if (hardware.CheckBoardVersion() != daisy::DaisySeed::BoardVersion::DAISY_SEED_3)
        fail(LedStatus::WrongBoard);
    SystemCoreClockUpdate(); // Reads RCC, including CM7 D1 prescaler; not a literal 480 MHz.
    if (!makeBudget(SystemCoreClock, rate, hardware.AudioBlockSize(), budget))
        fail(LedStatus::InvalidConfiguration);
    counterAvailable = enableCounter();
    if (!capture.initialize(budget, counterAvailable)) fail(LedStatus::InvalidConfiguration);
    hardware.SetLed(true);
}
inline void load(const parable::Patch& patch) {
    const uint32_t startMs = daisy::System::GetNow();
    const uint32_t start = counterAvailable ? DWT->CYCCNT : 0;
    const auto status = audioEngine->load(patch); // Stopped; includes Rat ctor + second configure.
    __DSB();
    const uint32_t end = counterAvailable ? DWT->CYCCNT : 0;
    loadMs = daisy::System::GetNow() - startMs;
    loadCyclesValid = counterAvailable && startupIntervalValid(budget.coreHz, loadMs);
    loadCycles = loadCyclesValid ? elapsedCycles(start, end) : 0;
    if (status != parable::Status::Ok) fail(LedStatus::LoadFailed);
}
struct Token { uint32_t start; bool recording; };
inline Token begin() noexcept {
    const bool recording = capture.recording(); // ISR writer only until frozen publication.
    return {recording && counterAvailable ? DWT->CYCCNT : 0, recording};
}
inline void end(Token token, size_t frames) noexcept {
    if (!token.recording) return;
    __DSB(); // Include completion of the output writes in the measured body.
    const uint32_t end = counterAvailable ? DWT->CYCCNT : 0;
    capture.observe(token.start, end, static_cast<uint32_t>(frames));
    // This bounded integer bookkeeping/publication tail and the return are outside
    // the timestamp window. SDK DMA conversion/ISR entry/exit are also excluded.
}
inline void formatReport(const Summary& s) {
#ifdef PARABLE_RAT_LINK_PROBE
#ifdef PARABLE_RAT_BENCH_1X
    constexpr const char* image = "RAT_COMPOSITE_1X Gain(1.8)->Rat(.35/.5/.7)->Lowpass(1800Hz) trim=.8";
#else
    constexpr const char* image = "RAT_COMPOSITE_4X Gain(1.8)->Rat(.35/.5/.7)->Lowpass(1800Hz) trim=.8";
#endif
#else
    constexpr const char* image = "MONO_BYPASS trim=1";
#endif
    // ARM newlib-nano printf does not support the %llu modifier in this
    // configuration. Print a bounded 32-bit ratio instead of shifting varargs.
    const uint64_t ratio = ratioPpm(s.maxCycles, budget.deadlineCycles);
    const uint32_t printableRatio = ratio > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(ratio);
    const int n = std::snprintf(report, sizeof(report),
        "\r\nPAR009 %s\r\nwindow=FIRST_30000_CALLBACKS_FROZEN_USB_STARTED_AFTER_CAPTURE\r\n"
        "core_hz_rcc=%lu configured_rate_millihz=%lu block_frames=%lu estimated_deadline_cycles=%lu\r\n"
        "counter_enabled=%u blocks=%lu valid_blocks=%lu max_valid_callback_body_cycles=%lu last_valid_cycles=%lu\r\n"
        "at_or_above_estimated_deadline=%lu invalid_intervals=%lu frame_mismatches=%lu max_valid_ratio_ppm=%lu\r\n"
        "stopped_load_ms=%lu stopped_load_cycles_valid=%u stopped_load_cycles=%lu engine_bytes=%u\r\n"
        "excludes=measurement_tail_return_SDK_ISR_and_postcapture_USB\r\n"
        "interval_contract=each_callback_less_than_2^31_cycles_multiwrap_hangs_not_resolvable\r\n"
        "glitches=LISTENER_OBSERVATION_REQUIRED heap_stack_highwater=NOT_MEASURED\r\n",
        image, static_cast<unsigned long>(budget.coreHz), static_cast<unsigned long>(budget.rateMilliHz),
        static_cast<unsigned long>(budget.frames), static_cast<unsigned long>(budget.deadlineCycles),
        unsigned(counterAvailable), static_cast<unsigned long>(s.blocks), static_cast<unsigned long>(s.validBlocks),
        static_cast<unsigned long>(s.maxCycles), static_cast<unsigned long>(s.lastCycles),
        static_cast<unsigned long>(s.atOrAboveDeadline), static_cast<unsigned long>(s.invalidIntervals),
        static_cast<unsigned long>(s.frameMismatches),
        static_cast<unsigned long>(printableRatio),
        static_cast<unsigned long>(loadMs), unsigned(loadCyclesValid), static_cast<unsigned long>(loadCycles),
        unsigned(sizeof(parable::Engine)));
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(report)) fail(LedStatus::InvalidConfiguration);
    reportBytes = static_cast<size_t>(n);
}
// CONTROL SIDE ONLY, after capture. Mask only around the pinned CDC state check
// and one nonblocking enqueue: unplug/reset IRQ cannot null pClassData mid-call.
// The immutable buffer remains valid even if a transfer stays busy indefinitely.
inline void tryReport() noexcept {
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData) {
        const auto* cdc = static_cast<const USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData);
        if (!cdc->TxState)
            hardware.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(report), reportBytes);
    }
    __set_PRIMASK(mask);
}
[[noreturn]] inline void run(daisy::AudioHandle::AudioCallback callback) {
#ifdef PARABLE_RAT_LINK_PROBE
    constexpr auto readyStatus = LedStatus::RatReady;
#else
    constexpr auto readyStatus = LedStatus::BypassReady;
#endif
    audioAttempted = true;
    if (hardware.audio_handle.Start(callback) != daisy::AudioHandle::Result::OK)
        fail(LedStatus::AudioStartFailed);
    // Pinned Start() does not propagate every lower-level DMA error. Capture
    // timeout is an additional diagnostic, never proof of physical audio.
    const uint32_t started = daisy::System::GetNow();
    uint32_t lastReport = started;
    bool usbStarted = false;
    while (true) {
        const uint32_t now = daisy::System::GetNow();
        const Summary* frozen = capture.snapshot(); // Acquire; never inspect mutable ISR state.
        if (!frozen && now - started >= 60000) fail(LedStatus::CaptureTimeout);
        if (frozen && !usbStarted) {
            formatReport(*frozen);
            hardware.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
            usbStarted = true;
        }
        if (usbStarted && now - lastReport >= 1000) {
            tryReport(); lastReport = now;
        }
        hardware.SetLed(statusLed(counterAvailable ? readyStatus : LedStatus::CounterUnavailable, now));
        daisy::System::Delay(10);
    }
}
} // namespace first_boot_runtime
