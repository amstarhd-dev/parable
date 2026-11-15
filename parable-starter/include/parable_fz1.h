#pragma once
// SF015 standalone, documented original-FZ1-style. No Engine/patch admission.
#include <array>
#include <cstddef>
#include <cstdint>
namespace parable::dsp {
struct Fz1Config {
    float attack=.5f,volume=.5f; // electrical resistance/divider fractions, stopped only
    bool openBiasFeed=false; // ONLY fault: physical470k Q2 feed disconnected
    double supplyVolts=3,sourceOhms=1000,loadOhms=1e6;
    double leakageScale=1; // offline ISC sensitivity, identical in paired auditions
};
class Fz1 {
public:
    static constexpr std::size_t kNodes=6,kPorts=6,kCaps=4,kTaps=385,kUpdates=24;
    enum Node:std::size_t {B1,E1,B2,C2,B3,C3};
    using Nodes=std::array<double,kNodes>;
    using Ports=std::array<double,kPorts>;
    // Stopped control only. Diagnostic1x permits up to1536k; normal8x44.1..96k;2x/4x comparisons.
    bool prepare(float rate,Fz1Config={},unsigned factor=8) noexcept;
    void reset() noexcept;
    void stop() noexcept {running_=false;}
    float process(float inputVolts) noexcept; // normalized physical jack/4, fixed headroom
    Nodes nodes() const noexcept;
    Nodes biasNodes() const noexcept;
    double capacitorVolts(std::size_t c) const noexcept {return c<kCaps?-capVolts_[c]:0;}
    double collectorAmps(std::size_t q) const noexcept {return q<3?currents_[2*q+1]:0;}
    double baseAmps(std::size_t q) const noexcept {return q<3?-(currents_[2*q]+currents_[2*q+1]):0;}
    double jackVolts() const noexcept {return jack_;} // internal-sample, before FIR/headroom
    double inputAmps() const noexcept {return inputAmps_;}
    double residualVolts() const noexcept {return residual_;}
    uint32_t failures() const noexcept {return failures_;}
    uint32_t inputClamps() const noexcept {return inputClamps_;}
    uint32_t outputClamps() const noexcept {return outputClamps_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t evaluations() const noexcept {return evaluations_;}
    uint32_t solves() const noexcept {return solves_;}
    uint32_t maximumEvaluations() const noexcept {return maximum_;}
private:
    using Matrix=std::array<Ports,kPorts>;
    struct Capacitor {Nodes a{};double farads=0,series=0,g=0,z=0;};
    bool matrices(bool dc) noexcept;
    bool solve(Ports&,const Ports&,unsigned,double&,unsigned&,Ports&) const noexcept;
    Nodes reconstruct(const Nodes&,const Ports&) const noexcept;
    void resetCircuit() noexcept;
    double sample(double) noexcept;
    std::array<Capacitor,kCaps> caps_{};
    std::array<std::array<double,kCaps>,kNodes> history_{};
    std::array<Ports,kNodes> current_{};
    Matrix impedance_{};
    Nodes open_{},bias_{},nodes_{};
    Ports previous_{},biasPorts_{},biasCurrents_{},currents_{};
    std::array<double,kCaps> capVolts_{},capBias_{};
    std::array<double,3> bcHistory_{};
    std::array<float,kTaps> fir_{},down_{};
    std::array<float,97> up_{};
    Fz1Config config_{};
    double rate_=0,bcG_=0,jackOhms_=0,jack_=0,inputAmps_=0,residual_=0;
    unsigned factor_=8,taps_=385,upSize_=49,upIndex_=0,downIndex_=0;
    uint32_t failures_=0,inputClamps_=0,outputClamps_=0,invalid_=0;
    uint32_t evaluations_=0,solves_=0,maximum_=0;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Fz1)<=12288,"SF015 complete standalone state ceiling");
} // namespace parable::dsp
