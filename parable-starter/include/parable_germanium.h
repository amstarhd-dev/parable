#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace parable::dsp {
enum class GermaniumRate { Reference1x, Oversampled4x };
struct GermaniumControls { float fuzz=.8f, volume=.5f; };
// Controlled variations of one published AC128 fitted sample, not a part library.
struct GermaniumDevice {
    double forwardGain=229.6, reverseGain=14.66;
    double saturationAmps=20.66e-6, beLeakageAmps=2.190e-6, bcLeakageAmps=7.546e-6;
};
struct GermaniumCircuit {
    double supplyVolts=9, sourceOhms=1000; // Physical supply is -supplyVolts.
    double inputFarads=2.2e-6, collector1Ohms=33e3, feedbackOhms=100e3;
    double fuzzOhms=1000, bypassFarads=20e-6;
    double collector2Ohms=8200, supplyTapOhms=470;
    double outputFarads=10e-9, volumeOhms=500e3, loadOhms=1e6;
    GermaniumDevice q1{},q2{};
};
struct GermaniumNodes {
    // Physical signs: battery positive = audio ground0, negative rail -9V.
    double base1=0, collector1=0, emitter2=0, collector2=0, tap=0, jack=0;
    double inputAmps=0, residualAmps=0;
    unsigned iterations=0;
    bool converged=true;
};
class Germanium {
public:
    Germanium() noexcept;
    // Stopped audio only. Rejects invalid/nonconvergent preparation unchanged.
    // 4x base rates32..96kHz; scientific1x comparison also permits192kHz.
    bool configure(float rate,GermaniumControls controls={},const GermaniumCircuit& circuit={},
                   GermaniumRate mode=GermaniumRate::Oversampled4x) noexcept;
    void reset() noexcept;
    float process(float inputVolts) noexcept; // Physical jack volts; no makeup gain.
    void processBlock(const float* input,float* output,std::size_t count) noexcept;
    const GermaniumNodes& lastNodes() const noexcept {return nodes_;}
    const GermaniumNodes& bias() const noexcept {return biasNodes_;}
    const GermaniumCircuit& circuit() const noexcept {return circuit_;}
    std::uint64_t failedSolves() const noexcept {return failed_;}
    std::uint64_t solverCalls() const noexcept {return calls_;}
    std::uint64_t loopEvaluations() const noexcept {return evaluations_;}
    std::uint64_t containedOutputs() const noexcept {return contained_;}
    unsigned maximumLoopEvaluations() const noexcept {return maximum_;}
private:
    explicit Germanium(bool) noexcept {}
    using Vector=std::array<double,4>;
    using Matrix=std::array<Vector,4>;
    bool solve(Vector& x,const Vector& rhs,const Matrix& passive,unsigned budget,
               double& residual,unsigned& loops) const noexcept;
    void coefficients(bool dc) noexcept;
    void resetCircuit() noexcept;
    double circuitSample(double physicalInput) noexcept;
    static constexpr std::size_t kTaps=193;
    GermaniumCircuit circuit_{};
    GermaniumControls controls_{};
    GermaniumRate mode_=GermaniumRate::Oversampled4x;
    double rate_=192000,gin_=0,gcapIn_=0,gcapFuzz_=0,gcapOut_=0;
    double emitG_=0,emitHistory_=0,wiperA_=0,wiperZ_=0;
    double tapA_=0,tapSupply_=0,tapHistory_=0,bottom_=0,volumeR_=0,collectorG_=0;
    double zIn_=0,zFuzz_=0,zOut_=0,zBc1_=0,zBc2_=0;
    Matrix passive_{};
    Vector previous_{},bias_{};
    GermaniumNodes nodes_{},biasNodes_{};
    std::array<float,kTaps> fir_{},down_{};
    std::array<float,(kTaps+3)/4> up_{};
    std::size_t upIndex_=0,downIndex_=0;
    std::uint64_t failed_=0,calls_=0,evaluations_=0,contained_=0;
    unsigned maximum_=0;
};
} // namespace parable::dsp
