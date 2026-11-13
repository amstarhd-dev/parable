#pragma once
// SF-009 standalone V3 circuit experiment; no Engine/canonical registration.
#include <array>
#include <cstddef>
#include <cstdint>
namespace parable::dsp {
struct MuffConfig {bool highR15=false;float sustain=.75f,tone=.5f,volume=.5f;};
enum class MuffRate:uint8_t {Oversampled4x,Reference1x};
class Muff {
public:
    static constexpr std::size_t kNodes=17,kPorts=10,kCaps=13,kTaps=193,kUpdates=24;
    // Source drawing numbering, reversed relative to the common Q1->Q4 convention.
    enum Node:std::size_t {B4,C4,E4,B3,C3,E3,B2,C2,E2,B1,C1,E1,Sustain,ToneLow,ToneHigh,D3,D2};
    using Nodes=std::array<double,kNodes>;
    using Ports=std::array<double,kPorts>;
    bool prepare(float rate,MuffConfig={},MuffRate=MuffRate::Oversampled4x) noexcept;
    void reset() noexcept; // stopped control; deterministic DC precharge/history.
    void stop() noexcept {running_=false;}
    float process(float input) noexcept;
    const Nodes& nodes() const noexcept {return nodes_;} // audio thread only.
    const Nodes& biasNodes() const noexcept {return bias_;}
    double capacitorVolts(std::size_t index) const noexcept {return index<kCaps?capVolts_[index]:0;} // C1..C13 =>0..12.
    double diodeAmps(std::size_t stage) const noexcept {return stage<2?diodes_[stage]:0;}
    double jackVolts() const noexcept {return jack_;}
    double toneVolts() const noexcept {return tone_;}
    double inputAmps() const noexcept {return inputAmps_;}
    double residualVolts() const noexcept {return residual_;}
    uint32_t failures() const noexcept {return failures_;}
    uint32_t outputClamps() const noexcept {return outputClamps_;}
    uint32_t inputClamps() const noexcept {return inputClamps_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t evaluations() const noexcept {return evaluations_;}
    uint32_t solves() const noexcept {return solves_;}
    uint32_t maximumEvaluations() const noexcept {return maximum_;}
private:
    using Matrix=std::array<Ports,kPorts>;
    struct Capacitor {Nodes a{};double farads=0,series=0,g=0,z=0;};
    bool matrices(bool dc) noexcept;
    bool solve(Ports&,const Ports&,uint32_t,double&,uint32_t&,Ports&) const noexcept;
    Nodes reconstruct(const Nodes&,const Ports&) const noexcept;
    void resetCircuit() noexcept;
    double sample(double input) noexcept;
    std::array<Capacitor,kCaps> caps_{};
    std::array<std::array<double,kCaps>,kNodes> history_{};
    std::array<Ports,kNodes> current_{};
    Matrix impedance_{};
    Nodes open_{},bias_{},nodes_{};
    Ports previous_{},biasPorts_{},biasCurrents_{};
    std::array<double,kCaps> capVolts_{},capBias_{};
    std::array<double,2> diodes_{};
    std::array<float,kTaps> fir_{},down_{};
    std::array<float,(kTaps+3)/4> up_{};
    MuffConfig config_{};
    MuffRate mode_=MuffRate::Oversampled4x;
    double rate_=0,bottom_=0,jack_=0,tone_=0,inputAmps_=0,residual_=0;
    uint32_t upIndex_=0,downIndex_=0,failures_=0,outputClamps_=0,inputClamps_=0,invalid_=0;
    uint32_t evaluations_=0,solves_=0,maximum_=0;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Muff)<=16384,"SF009 complete standalone state ceiling");
} // namespace parable::dsp
