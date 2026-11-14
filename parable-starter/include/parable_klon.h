#pragma once
// SF-011 experimental S698 signal circuit; no Engine/patch registration.
#include <array>
#include <cstddef>
#include <cstdint>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Klon requires IEEE finite checks"
#endif
namespace parable::dsp {
struct KlonControls {float gain=.5f,treble=.5f,level=.5f;bool faultyContact=false;};
enum class KlonRate {Oversampled12x,Reference1x};
class Klon {
public:
    static constexpr std::size_t kNodes=21,kCaps=17,kUnknowns=5,kFactor=12,kTaps=577,kUpdates=24,kSearch=8;
    enum Node : std::size_t {Input,Buffer,Pre,CleanUpper,DrivePlus,Feedback,DriveMinus,DriveOut,Clip,PostClip,CleanWiper,FF1,SumMinus,SumOut,ToneMinus,ToneOut,LevelTop,BypassHP,Bypass,Jack,Vref};
    using Nodes=std::array<double,kNodes>;
    using Unknowns=std::array<double,kUnknowns>;
    // One stopped control owner; no concurrent prepare/reset and audio access.
    // Failed preparation preserves the complete old state.
    bool prepare(float sampleRate,KlonControls={},KlonRate=KlonRate::Oversampled12x) noexcept;
    void stop() noexcept {running_=false;}
    void reset() noexcept;
    float process(float inputVolts) noexcept;
    const Nodes& nodes() const noexcept {return nodes_;} // same audio thread only
    const Nodes& biasNodes() const noexcept {return bias_;}
    double capacitorVolts(std::size_t k) const noexcept {return k<kCaps?capVolts_[k]:0;}
    double capacitorAmps(std::size_t k) const noexcept {return k<kCaps?capAmps_[k]:0;}
    double amplifierVolts(std::size_t k) const noexcept {return k<4?previous_[1+k]:0;} // relative4.5V
    double diodeAmps(std::size_t k) const noexcept {return k<2?diodes_[k]:0;} // D2,D3 anode->cathode
    const std::array<double,3>& mixCurrents() const noexcept {return mix_;} // drive,clean1,clean2 into real sum node
    const std::array<double,3>& feedbackContributions() const noexcept {return paths_;} // diagnostic volts, before Tone
    double mixResidualAmps() const noexcept {return kcl_;}
    double residualVolts() const noexcept {return residual_;}
    uint32_t failures() const noexcept {return failures_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t inputClamps() const noexcept {return inputClamps_;}
    uint32_t outputClamps() const noexcept {return outputClamps_;}
    uint32_t evaluations() const noexcept {return evaluations_;}
    uint32_t solves() const noexcept {return solves_;}
    uint32_t maximumEvaluations() const noexcept {return maximum_;}
    uint32_t railSamples() const noexcept {return rails_;}
    uint32_t slewSamples() const noexcept {return slews_;}
private:
    struct Cap {Nodes a{};double farads=0,series=0,g=0,z=0;};
    using Matrix=std::array<Unknowns,kUnknowns>;
    bool matrices(bool dc) noexcept;
    Nodes reconstruct(const Nodes&,const Unknowns&,double,bool) const noexcept;
    bool evaluate(const Unknowns&,const Nodes&,bool,Unknowns&,Matrix&,double&,std::array<double,2>&) const noexcept;
    bool solve(Unknowns&,const Nodes&,bool,double&,uint32_t&,double&,std::array<double,2>&) const noexcept;
    void resetCircuit() noexcept;
    double sample(double) noexcept;
    float coefficient(std::size_t tap) const noexcept {return fir_[tap<fir_.size()?tap:kTaps-1-tap];}
    std::array<Cap,kCaps> caps_{};
    std::array<std::array<double,kCaps>,kNodes> history_{};
    std::array<std::array<double,4>,kNodes> drive_{};
    Nodes current_{},nodes_{},bias_{};
    Unknowns previous_{},biasUnknowns_{};
    std::array<double,4> oldU_{};
    std::array<double,kCaps> capBias_{},capVolts_{},capAmps_{};
    std::array<double,3> mix_{},paths_{},oldMix_{};
    std::array<double,2> diodes_{};
    std::array<float,(kTaps+1)/2> fir_{};
    std::array<float,kTaps> down_{};
    std::array<float,(kTaps+kFactor-1)/kFactor> up_{};
    double rate_=0,biasCurrent_=0,residual_=0,kcl_=0;
    uint32_t upIndex_=0,downIndex_=0,failures_=0,invalid_=0,inputClamps_=0,outputClamps_=0;
    uint32_t evaluations_=0,solves_=0,maximum_=0,rails_=0,slews_=0;
    KlonControls controls_{};
    KlonRate mode_=KlonRate::Oversampled12x;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Klon)<=12288,"SF011 standalone state budget");
} // namespace parable::dsp
