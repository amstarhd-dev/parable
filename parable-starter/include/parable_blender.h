#pragma once
// SF016 standalone laboratory circuit. No Engine/canonical registration.
#include <array>
#include <cstddef>
#include <cstdint>
namespace parable::dsp {
struct BlenderConfig {
    bool d1Open=false,boost=false,effect=true;
    float sustain=.5f,tone=.5f,blend=.5f,level=.5f;
    float supplyVolts=9,temperatureC=26.85f,beta=350,sourceOhms=1000,loadOhms=1000000,junctionScale=1;
};
enum class BlenderRate:uint8_t {Direct=1,X2=2,X4=4,X8=8};
class Blender {
public:
    static constexpr std::size_t kNodes=31,kPorts=14,kCaps=30,kTaps=385,kUpdates=32;
    // c1 etc mean transistor collectors, not capacitor reference designators.
    enum Node:std::size_t {Input,B1,C1,E1,E2,Feedback,SustainTop,Dry,SustainWiper,
        B3,C3,E3,CollectorDiode,EmitterDiode,B4,C4,E4,Clip,ToneIn,ToneLow,ToneDark,
        ToneBright,ToneWiper,LevelTop,LevelWiper,B5,C5,E5,Wet,BlendWiper,Jack};
    using Nodes=std::array<double,kNodes>;
    using Ports=std::array<double,kPorts>;
    bool prepare(float rate,BlenderConfig={},BlenderRate=BlenderRate::X8) noexcept;
    void stop() noexcept {running_=false;}
    void reset() noexcept; // stopped control; precharged DC, no power-up claim.
    float process(float input) noexcept;
    const Nodes& nodes() const noexcept {return nodes_;} // same audio owner only.
    const Nodes& biasNodes() const noexcept {return bias_;}
    double capacitorVolts(std::size_t c) const noexcept {return c<kCaps?capVolts_[c]:0;}
    double capacitorAmps(std::size_t c) const noexcept {return c<kCaps?capAmps_[c]:0;}
    // B,Ic,Ie into the intrinsic transistor, excluding declared external CBE/CBC.
    double transistorAmps(std::size_t q,std::size_t terminal) const noexcept {return q<5&&terminal<3?transistor_[q][terminal]:0;}
    // Intrinsic diode conduction+diffusion; branch adds package shunt/depletion C.
    double diodeAmps(std::size_t d) const noexcept {return d<4?diodes_[d]:0;}
    double diodeBranchAmps(std::size_t d) const noexcept {return d<4?branches_[d]:0;}
    double inputAmps() const noexcept {return inputAmps_;}
    double jackVolts() const noexcept {return jack_;}
    double residualVolts() const noexcept {return residual_;}
    uint32_t failures() const noexcept {return failures_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t inputClamps() const noexcept {return inputClamps_;}
    uint32_t outputClamps() const noexcept {return outputClamps_;}
    uint32_t evaluations() const noexcept {return evaluations_;}
    uint32_t solves() const noexcept {return solves_;}
    uint32_t maximumEvaluations() const noexcept {return maximum_;}
    unsigned factor() const noexcept {return factor_;}
private:
    static constexpr int Ground=-1,Rail=-2;
    using Matrix=std::array<Ports,kPorts>;
    struct Capacitor {int p=Ground,n=Ground;double farads=0,series=0,g=0,z=0,previous=0;};
    struct Evaluation {Ports i{},g{},q{},raw{};};
    bool matrices(bool dc) noexcept;
    bool evaluate(const Ports&,bool dc,Evaluation&) const noexcept;
    bool solve(Ports&,const Ports&,bool dc,double&,uint32_t&,Evaluation&) const noexcept;
    Nodes reconstruct(const Nodes&,const Ports&) const noexcept;
    Ports ports(const Nodes&,bool physical=false) const noexcept;
    double node(const Nodes&,int,bool physical=false) const noexcept;
    void observe(const Ports&,const Ports&,const Ports&) noexcept;
    void resetCircuit() noexcept;
    double sample(double input) noexcept;
    std::array<Capacitor,kCaps> caps_{};
    std::array<std::array<double,kCaps>,kNodes> history_{};
    std::array<Ports,kNodes> current_{};
    Matrix impedance_{};
    Nodes open_{},source_{},bias_{},nodes_{};
    Ports previous_{},biasPorts_{},biasCurrents_{},biasRaw_{},biasCharge_{};
    std::array<double,kCaps> capVolts_{},capBias_{},capAmps_{};
    std::array<double,4> chargeZ_{},chargePrevious_{},diodes_{},branches_{};
    std::array<std::array<double,3>,5> transistor_{};
    std::array<float,kTaps> fir_{},down_{};
    std::array<float,49> up_{};
    BlenderConfig config_{};
    double rate_=0,vt_=0,si_=0,ge_=0,theta_=0,diodeG_=0,jack_=0,inputAmps_=0,residual_=0;
    uint32_t upIndex_=0,downIndex_=0,failures_=0,invalid_=0,inputClamps_=0,outputClamps_=0;
    uint32_t evaluations_=0,solves_=0,maximum_=0;
    unsigned factor_=8,taps_=kTaps;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Blender)<=32768,"SF016 standalone state budget");
} // namespace parable::dsp
