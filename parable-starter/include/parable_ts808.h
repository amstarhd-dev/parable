#pragma once
// SF-010 experimental standalone circuit. No patch/Engine registration.
#include <array>
#include <cstddef>
#include <cstdint>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "TS808 requires IEEE finite checks"
#endif
namespace parable::dsp {
struct Ts808Controls { float drive=.5f, tone=.5f, level=.5f; bool openD2=false; };
// Reference1x is diagnostic: coarse grids can fail the recorded hard stress;
// normal playback uses the validated16x path and visible failure counters.
enum class Ts808Rate { Oversampled16x, Reference1x };
class Ts808 {
public:
    static constexpr std::size_t kNodes=12, kJunctions=5, kUnknowns=7, kCaps=9, kFactor=16, kTaps=769, kUpdates=32, kSearch=12;
    enum Node : std::size_t { B1,E1,Plus,Minus,Clip,TonePlus,ToneMinus,ToneOut,LevelTop,B3,E3,Jack };
    using Nodes=std::array<double,kNodes>;
    // Five nonlinear junction unknowns plus two exactly eliminated amplifier values.
    using Unknowns=std::array<double,kUnknowns>;
    using Currents=std::array<double,kJunctions>;
    // Stopped control only. Failed preparation preserves the old state exactly.
    bool prepare(float rate, Ts808Controls={}, Ts808Rate=Ts808Rate::Oversampled16x) noexcept;
    void stop() noexcept { running_=false; }
    void reset() noexcept;
    float process(float inputVolts) noexcept;
    const Nodes& nodes() const noexcept { return nodes_; } // same audio thread only
    const Nodes& biasNodes() const noexcept { return bias_; }
    double internalVolts(std::size_t amp) const noexcept { return amp<2 ? previous_[5+amp] : 0; } // relative4.5V
    double capacitorVolts(std::size_t cap) const noexcept { return cap<kCaps ? capVolts_[cap] : 0; }
    double diodeAmps(std::size_t diode) const noexcept { return diode<2 ? diodes_[diode] : 0; } // D1,D2 anode->cathode
    double residualVolts() const noexcept { return residual_; }
    uint32_t failures() const noexcept { return failures_; }
    uint32_t outputClamps() const noexcept { return outputClamps_; }
    uint32_t inputClamps() const noexcept { return inputClamps_; }
    uint32_t invalidInputs() const noexcept { return invalid_; }
    uint32_t evaluations() const noexcept { return evaluations_; }
    uint32_t solves() const noexcept { return solves_; }
    uint32_t maximumEvaluations() const noexcept { return maximum_; }
    uint32_t railSamples() const noexcept { return railSamples_; } // physical macro, NOT numerical fallback
    uint32_t slewSamples() const noexcept { return slewSamples_; }
private:
    struct Capacitor { Nodes a{}; double farads=0,series=0,g=0,z=0; };
    bool matrices(bool dc) noexcept;
    Nodes reconstruct(const Nodes&, const Unknowns&, const Currents&, bool dc) const noexcept;
    using Matrix=std::array<Currents,kJunctions>;
    bool evaluate(Unknowns&,const Nodes&,bool,Unknowns&,Matrix&,Currents&,std::array<double,2>&) const noexcept;
    bool solve(Unknowns&, const Nodes&, bool dc, double&, uint32_t&, Currents&, std::array<double,2>&) const noexcept;
    void resetCircuit() noexcept;
    double sample(double) noexcept;
    float coefficient(std::size_t tap) const noexcept {return fir_[tap<fir_.size()?tap:kTaps-1-tap];}
    std::array<Capacitor,kCaps> caps_{};
    std::array<std::array<double,kCaps>,kNodes> history_{};
    std::array<Currents,kNodes> current_{};
    std::array<std::array<double,2>,kNodes> drive_{};
    Nodes bias_{},nodes_{};
    Unknowns previous_{},biasUnknowns_{};
    Currents biasCurrents_{};
    std::array<double,kCaps> capVolts_{},capBias_{};
    std::array<double,2> diodes_{},oldU_{};
    std::array<float,(kTaps+1)/2> fir_{};
    std::array<float,kTaps> down_{};
    std::array<float,(kTaps+kFactor-1)/kFactor> up_{};
    Ts808Controls controls_{};
    Ts808Rate mode_=Ts808Rate::Oversampled16x;
    double rate_=0,residual_=0;
    uint32_t upIndex_=0,downIndex_=0,failures_=0,outputClamps_=0,inputClamps_=0,invalid_=0;
    uint32_t evaluations_=0,solves_=0,maximum_=0,railSamples_=0,slewSamples_=0;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Ts808)<=8192,"SF010 standalone state budget");
} // namespace parable::dsp
