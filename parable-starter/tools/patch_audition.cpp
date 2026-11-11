// Host evidence through the real canonical decode -> prepare -> Engine path.
#include "parable_prepare.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

namespace {
std::string read(const std::string& name) {
    std::ifstream f(std::string(PATCH_FIXTURES) + "/" + name + ".json", std::ios::binary);
    lab::require(bool(f), "Cannot read canonical-input fixture");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void audition(const std::filesystem::path& output) {
    const auto input = soundforge::syntheticPlucks(); // Owned 9 s, 48 kHz, peak .15.
    const auto dry = soundforge::stats(input.samples);
    const char* fixtures[] = {"empty", "rat", "rat-composite", "rat-reversed"};
    const char* names[] = {"00_dry", "01_rat", "02_gain_rat_lowpass", "03_lowpass_rat_gain"};
    const char* chains[] = {"input -> output", "Rat[id=7]", "Gain[id=40] -> Rat[id=7] -> OnePoleLowpass[id=3]",
                           "OnePoleLowpass[id=3] -> Rat[id=7] -> Gain[id=40]"};
    std::filesystem::create_directories(output);
    std::ofstream report(output / "renders.csv");
    report << std::setprecision(12)
        << "# Owned synthetic plucks/strums; not recorded guitar. 1 engine unit=1 model input V; RAT input bounded +/-4 V.\n"
        << "# Rat returns jack V/4; nominal circuit, 4x FIR, 48 base-sample FIR delay plus circuit; no alignment or RMS matching.\n"
        << "# Raw columns are the input excitation; pretrim columns reconstruct output/trim only after verifying no final clipping.\n"
        << "file,rate,frames,chain,distortion,filter,volume,raw_peak,raw_rms,raw_dc,pretrim_peak,pretrim_rms,pretrim_dc,output_trim,master_volume,post_gain,processed_peak,processed_rms,processed_dc\n";
    std::array<std::vector<float>, 4> signals;
    double standalonePeakError = 0;
    for (size_t i = 0; i < signals.size(); ++i) {
        const auto decoded = parable::patch::Codec::decode(read(fixtures[i]));
        const auto* document = std::get_if<parable::patch::ValidatedPatch>(&decoded);
        lab::require(document != nullptr, "Audition patch decode failed");
        const auto prepared = parable::preparation::prepare(*document, float(input.rate));
        const auto* patch = std::get_if<parable::Patch>(&prepared);
        lab::require(patch != nullptr, "Audition patch preparation failed");
        parable::Engine engine(float(input.rate));
        lab::require(engine.load(*patch) == parable::Status::Ok, "Stopped Engine load failed");
        parable::dsp::Rat standalone;
        lab::require(standalone.configure(float(input.rate), {.35f, .5f, .7f}), "Standalone configuration failed");
        auto& result = signals[i]; result.reserve(input.samples.size());
        for (float x : input.samples) {
            const float y = engine.process(x); result.push_back(y);
            if (i == 1) standalonePeakError = std::max(standalonePeakError,
                std::abs(double(y) - standalone.process(std::clamp(x, -4.f, 4.f)) * patch->outputTrim));
        }
        const auto processed = soundforge::stats(result);
        lab::require(processed.peak < .99, "Final safety clamp may conceal pretrim level; refuse this evidence");
        const double trim = patch->outputTrim;
        lab::require(trim > 0, "Cannot reconstruct pretrim level with zero trim");
        report << names[i] << ".wav," << input.rate << ',' << input.samples.size() << ',' << chains[i] << ','
               << (i ? ".35,.5,.7," : "n/a,n/a,n/a,") << dry.peak << ',' << dry.rms << ',' << dry.mean << ','
               << processed.peak / trim << ',' << processed.rms / trim << ',' << processed.mean / trim << ','
               << trim << ",1,1," << processed.peak << ',' << processed.rms << ',' << processed.mean << '\n';
        lab::write(output / (std::string(names[i]) + ".wav"), {input.rate, result});
        std::ofstream canonical(output / (std::string(names[i]) + ".patch.json"), std::ios::binary);
        canonical << parable::patch::Codec::encode(*document); canonical.flush();
        lab::require(bool(canonical), "Cannot write authoritative audition patch");
    }
    lab::require(standalonePeakError == 0, "Adapter changed standalone RAT output");
    double orderSquared = 0;
    for (size_t i = 0; i < input.samples.size(); ++i) {
        const double d = double(signals[2][i]) - signals[3][i]; orderSquared += d * d;
    }
    const double orderRms = std::sqrt(orderSquared / input.samples.size());
    lab::require(orderRms > .001, "Reversed nonlinear chain did not differ as expected");
    std::ofstream checks(output / "checks.txt");
    checks << std::setprecision(12) << "standalone_vs_integrated_max_absolute_error=" << standalonePeakError
           << "\nforward_vs_reverse_rms_difference=" << orderRms << '\n';
    std::ofstream listen(output / "LISTEN.txt");
    listen << "PAR-008: real canonical decode -> prepare -> stopped Engine -> PCM16.\n"
        << "Four owned synthetic 9-second, mono 48 kHz WAVs: dry, nominal RAT only, Gain -> RAT -> Lowpass, reversed order.\n"
        << "Gain=1.8; cutoff=1800 Hz; RAT distortion=.35/filter=.5/volume=.7; nominal components/4x.\n"
        << "Engine input unit=1 model V, bounded +/-4 at RAT; output is existing jack V/4. No analog calibration.\n"
        << "Dry trim=1, other trims=.8, master Volume=1, file post gain=1. No automatic level or RMS matching.\n"
        << "Compare the last two files: gain before clipping changes nonlinear excitation; reversing the lowpass also changes what enters the nonlinearity.\n"
        << "Raw/pretrim/processed float peaks, RMS and DC precede undithered PCM16 quantization; exact patches accompany files.\n"
        << "Durations truncate tails, with no delay compensation. Start quietly. Not real guitar, founder musical approval, physical pedal fidelity or Daisy runtime proof.\n";
    report.flush(); checks.flush(); listen.flush();
    lab::require(bool(report) && bool(checks) && bool(listen), "Cannot finish evidence reports");
    std::cout << "Created 4 canonical Engine WAVs; standalone peak error " << standalonePeakError
              << "; order RMS difference " << orderRms << " in " << output << '\n';
}
}
int main(int argc, char** argv) {
    try {
        lab::require(argc == 3 && std::string(argv[1]) == "--output" && std::string(argv[2]).size() > 0,
                     "Usage: parable_patch_audition --output DIR");
        audition(argv[2]);
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
