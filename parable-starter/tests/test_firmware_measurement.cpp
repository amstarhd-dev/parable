#include "runtime_measurement.h"
#include "test_check.h"
#include <limits>

using namespace parable::first_boot;
int main() {
    Budget b;
    CHECK(makeBudget(480000000, 48000, 48, b));
    CHECK(b.deadlineCycles == 480000 && b.rateMilliHz == 48000000 && b.frames == 48);
    CHECK(makeBudget(216000000, 44100, 32, b));
    CHECK(b.deadlineCycles == 156734); // Independent floor(216e6 * 32 / 44100).
    const auto prior = b;
    for (float bad : {0.f, 7999.f, 192001.f, NAN, INFINITY}) CHECK(!makeBudget(480000000, bad, 48, b));
    CHECK(!makeBudget(0, 48000, 48, b));
    CHECK(!makeBudget(1, 192000, 1, b)); // Zero-cycle budget must fail, not divide later.
    CHECK(!makeBudget(480000000, 48000, 0, b));
    CHECK(!makeBudget(480000000, 48000, 129, b));
    CHECK(b.coreHz == prior.coreHz && b.deadlineCycles == prior.deadlineCycles); // Atomic rejection.
    CHECK(elapsedCycles(0xfffffff0, 0x10) == 32);
    CHECK(elapsedCycles(10, 20) == 10);
    CHECK(ratioPpm(120000, 480000) == 250000);
    CHECK(ratioPpm(480001, 480000) == 1000002);
    CHECK(ratioPpm(0x7fffffff, 1) == uint64_t{2147483647000000});
    CHECK(ratioPpm(10, 0) == 0);
    CHECK(startupIntervalValid(480000000, 8946));
    CHECK(!startupIntervalValid(480000000, 8947));
    CHECK(!startupIntervalValid(0, 1));
    CHECK(!startupIntervalValid(UINT32_MAX, UINT32_MAX));

    CHECK(makeBudget(480000000, 48000, 48, b));
    Capture capture;
    CHECK(capture.snapshot() == nullptr);
    CHECK(!capture.initialize(b, true, 0));
    CHECK(capture.initialize(b, true, 5));
    capture.observe(0xfffffff0, 0x10, 48);
    capture.observe(100, 480100, 48); // Exactly the estimated deadline.
    capture.observe(10, 480011, 48); // One cycle above.
    capture.observe(1, 1, 48); // Stopped counter is not zero utilization.
    CHECK(capture.snapshot() == nullptr);
    capture.observe(0, kAmbiguousCycles, 47);
    const auto* frozen = capture.snapshot(); CHECK(frozen != nullptr);
    CHECK(frozen->blocks == 5 && frozen->validBlocks == 3);
    CHECK(frozen->maxCycles == 480001 && frozen->lastCycles == 480001);
    CHECK(frozen->atOrAboveDeadline == 2 && frozen->invalidIntervals == 2 && frozen->frameMismatches == 1);
    // No later callback may modify a published result, including near-wrap data.
    for (unsigned i = 0; i < 1000; ++i) capture.observe(UINT32_MAX, 0, 48);
    CHECK(frozen->blocks == 5 && frozen->maxCycles == 480001 && frozen->invalidIntervals == 2);
    CHECK(!capture.recording());
    CHECK(capture.initialize(b, false, 1)); // Only with audio stopped.
    capture.observe(0, 100, 48);
    CHECK(capture.snapshot()->validBlocks == 0 && capture.snapshot()->invalidIntervals == 1);
    CHECK(capture.snapshot()->maxCycles == 0);
    CHECK(capture.initialize(b, true));
    for (uint32_t i = 0; i < kCaptureBlocks; ++i) capture.observe(0, i + 1, 48);
    CHECK(capture.snapshot()->blocks == 30000 && capture.snapshot()->maxCycles == 30000);

    for (unsigned pulses = 1; pulses <= 9; ++pulses) {
        const auto status = static_cast<LedStatus>(pulses);
        unsigned onMs = 0;
        for (uint32_t ms = 0; ms < 3000; ++ms) {
            if (statusLed(status, ms)) ++onMs;
            CHECK(statusLed(status, ms) == statusLed(status, ms + 3000));
        }
        CHECK(onMs == pulses * 100);
        CHECK(statusLed(status, 0) && !statusLed(status, 100));
        CHECK(!statusLed(status, 2900));
    }
    CHECK(statusLed(LedStatus::Initializing, 1000));
    CHECK(statusLed(static_cast<LedStatus>(255), UINT32_MAX));
    std::cout << "Firmware measurement/status simulation checks " << testChecks << '\n';
}
