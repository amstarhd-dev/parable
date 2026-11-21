
#pragma once
#include <array>
#include <cstddef>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "LabRAT requires IEEE finite checks"
#endif
namespace rat1 {
struct RatControls {float distortion=.5f, filter=.5f, volume=.75f;};
struct RatCircuit {
    double inputCapFarads=22e-9, inputBiasOhms=1e6, inputSeriesOhms=1000, inputShuntFarads=1e-9;
    double feedbackBassOhms=560, feedbackTrebleOhms=47;
    double feedbackBassFarads=4.7e-6, feedbackTrebleFarads=2.2e-6;
    double distortionPotOhms=100e3, feedbackCompFarads=100e-12;
    bool trebleLegConnected=true;
    double opampOpenLoopGain=200e3, opampGbwHz=1e6, opampSlewVoltsPerUs=.3;
    double supplyVolts=9, railMarginVolts=.5;
    double couplingFarads=4.7e-6, clippingSeriesOhms=1000;
    double diodePositiveIsAmps=2.52e-9, diodeNegativeIsAmps=2.52e-9;
    double diodeIdeality=1.752, thermalVolts=.02585;
    bool positiveDiodeConnected=true, negativeDiodeConnected=true;
    double filterFixedOhms=1500, filterPotOhms=100e3, filterFarads=3.3e-9;
    double gateCapFarads=22e-9, gateOhms=1e6;
    double outputCapFarads=10e-6, outputSeriesOhms=10e3, volumePotOhms=100e3, loadOhms=1e6;
};
enum class RatRate { Reference1x, Oversampled4x };
struct RatNodes {float linearOpamp=0, opamp=0, clip=0, tone=0, jackVolts=0;};
namespace rat_detail {
struct Filter {
    std::array<double,5> b{},a{},x{},y{};unsigned order=0;
    void prepare(const std::array<double,5>& numerator,const std::array<double,5>& denominator,double rate) noexcept;
    double process(double v) noexcept;
    void reset() noexcept {x.fill(0);y.fill(0);}
};
bool preparePreamp(Filter& input,Filter& gain,double rate,RatControls p,const RatCircuit& c) noexcept;
struct ClipStage {
    static constexpr std::size_t kTable=2049;
    std::array<float,kTable> inverse{};
#if 0
    // First-pass target profiling only: use hardware-FPU friendly state and
    // math in the clipping network; retain double-precision preamp filters.
    float sourceG=0,filterG=0,toneG=0,gateG=0,toneInv=0,toneInput=0,portR=1;
    float couplingR=0,clipR=0,gateR=0,outG=0,outR=0,bottomR=0;
    float zCoupling=0,zTone=0,zGate=0,zOut=0;
#else
    double sourceG=0,filterG=0,toneG=0,gateG=0,toneInv=0,toneInput=0,portR=1;
    double couplingR=0,clipR=0,gateR=0,outG=0,outR=0,bottomR=0;
    double zCoupling=0,zTone=0,zGate=0,zOut=0;
#endif
    float clip=0,tone=0;
    void prepare(double rate,RatControls control,const RatCircuit& c) noexcept;
#if 0
    float process(float v) noexcept;
    float lookup(float v) const noexcept;
#else
    double process(double v) noexcept;
    double lookup(double v) const noexcept;
#endif
    void reset() noexcept {zCoupling=zTone=zGate=zOut=0;clip=tone=0;}
};
}
// Laboratory processor, not registered in patch v1. See proposed ADR-003.
class Rat {
public:
    Rat() noexcept;
    // Stopped control side only; invalid controls/components/rate reject atomically.
    // Valid finite controls must be in [0,1]; no silent component clamping.
    bool configure(float rate,RatControls control={},const RatCircuit& circuit={},
                   RatRate mode=RatRate::Oversampled4x) noexcept;
    void reset() noexcept;
    float process(float input) noexcept;
    void processBlock(const float* input,float* output,std::size_t count) noexcept;
    RatNodes lastNodes() const noexcept {return nodes_;} // Same-thread diagnostics only.
    RatControls controls() const noexcept {return controls_;}
    RatCircuit circuit() const noexcept {return circuit_;}
#ifdef PARABLE_RAT_BENCH_FIR97
    // Diagnostic ONLY: 4x processing with a shorter anti-alias FIR.
    // The 97-tap two-pass interpolator/decimator has ~24 base-frame latency,
    // versus 48 for the original; match delays explicitly for audio A/B.
    static constexpr std::size_t kTaps=97,kFirDelay=24;
#else
    static constexpr std::size_t kTaps=193,kFirDelay=48;
#endif
private:
    double circuitSample(double v) noexcept;
    rat_detail::Filter input_,gain_;
    rat_detail::ClipStage clipper_;
    std::array<float,kTaps> fir_{},down_{};
    std::array<float,(kTaps+3)/4> up_{};
    std::size_t upIndex_=0,downIndex_=0;
    double previousOpamp_=0,slewStep_=1,rail_=4;
    RatControls controls_{};RatCircuit circuit_{};RatRate mode_=RatRate::Oversampled4x;
    RatNodes nodes_{};
};
} // namespace rat1
