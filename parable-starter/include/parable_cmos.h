#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace parable::dsp {
enum class CmosRate { Reference1x, Oversampled4x };
struct CmosControls { float drive=.5f, volume=.5f; };
struct CmosCircuit {
    double railVolts=9;
    double inputOhms=100e3, inputFarads=68e-9;
    double firstFeedbackOhms=100e3, drivePotOhms=1e6, firstFeedbackFarads=51e-12;
    double interstageFarads=33e-9;
    double secondFeedbackOhms=1e6, secondFeedbackFarads=100e-12;
    double secondBiasSiemens=0; // Leakage from gate 2 to ground; zero means open.
    double outputFarads=10e-6, volumePotOhms=10e3, loadOhms=1e6;
};
// One experimental Gemini malfunction: damaged gate-2 insulation/bias path to VSS.
// NOT a severity curve or an astrology rule. Prepared only, never toggled in audio.
inline constexpr double kGeminiBiasLeakOhms=220e3;
inline CmosCircuit geminiCircuit(bool biasLeak) noexcept {
    CmosCircuit c;c.secondBiasSiemens=biasLeak?1/kGeminiBiasLeakOhms:0;return c;
}
struct CmosNodes {
    double gate1=0, output1=0, gate2=0, output2=0, jackVolts=0, residualAmps=0;
    unsigned iterations=0;
    bool converged=true;
};

// Experimental two-inverter reduced signal path; GeminiCmos is a narrow Engine registration.
// configure/reset require stopped audio. Instances are neither shared nor atomic.
class Cmos {
public:
    Cmos() noexcept;
    bool configure(float rate, CmosControls controls={}, const CmosCircuit& circuit={},
                   CmosRate mode=CmosRate::Oversampled4x) noexcept;
    void reset() noexcept;
    float process(float inputVolts) noexcept;
    void processBlock(const float* input, float* output, std::size_t count) noexcept;
    const CmosNodes& lastNodes() const noexcept { return nodes_; }
    std::uint64_t failedSolves() const noexcept { return failed_; }
    std::uint64_t solverCalls() const noexcept { return calls_; }
    std::uint64_t loopEvaluations() const noexcept { return evaluations_; }
    unsigned maximumLoopEvaluations() const noexcept { return maximum_; }
    const CmosCircuit& circuit() const noexcept { return circuit_; }
    double biasVolts() const noexcept { return bias_; }
    double secondGateBiasVolts() const noexcept { return biasGate2_; }
    double secondOutputBiasVolts() const noexcept { return biasOutput2_; }
private:
    void resetState() noexcept;
    double circuitSample(double input) noexcept;
    static constexpr std::size_t kTaps=193;
    CmosCircuit circuit_{};
    CmosRate mode_=CmosRate::Oversampled4x;
    double rate_=192000, bias_=0, biasGate2_=0, biasOutput2_=0;
    double gin_=0, cfin_=0, gf1_=0, cf1_=0, link_=0, gf2_=0, cf2_=0;
    double gout_=0, rout_=0, bottom_=0, a1_=0, b1_=0, a2_=0, b2_=0;
    double zInput_=0, zFirst_=0, zLink_=0, zSecond_=0, zOutput_=0;
    double previous1_=0, previous2_=0;
    std::array<float,kTaps> fir_{}, down_{};
    std::array<float,(kTaps+3)/4> up_{};
    std::size_t upIndex_=0, downIndex_=0;
    CmosNodes nodes_{};
    std::uint64_t failed_=0;
    std::uint64_t calls_=0, evaluations_=0;
    unsigned maximum_=0;
};
} // namespace parable::dsp
