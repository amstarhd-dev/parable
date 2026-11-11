// USB-powered standalone LED/ROM-recovery gate. No codec, SDRAM, QSPI or Engine init.
// PC7 and CheckBoardVersion come from pinned MIT libDaisy's daisy_seed.cpp.
#include "daisy_seed.h"
#include "runtime_measurement.h"

static daisy::DaisySeed hardware;
int main() {
    daisy::System::Config config;
    config.Defaults();
    hardware.system.Init(config);
    hardware.led.Init(daisy::Pin(daisy::PORTC, 7), daisy::GPIO::Mode::OUTPUT);
    using namespace parable::first_boot;
    const auto status = hardware.CheckBoardVersion() == daisy::DaisySeed::BoardVersion::DAISY_SEED_3
        ? LedStatus::LedReady : LedStatus::WrongBoard;
    while (true) {
        hardware.SetLed(statusLed(status, daisy::System::GetNow()));
        daisy::System::Delay(10);
    }
}
