#pragma once
// SF018 standalone1978-family laboratory; no Engine/patch registration.
#include <array>
#include <cstdint>
namespace parable::dsp {
struct CapricornControls {float distortion=.5f,tone=.5f,level=.8f;bool openC4=false;};
struct CapricornProbe {float q2Volts=0,amplifierVolts=0,diodeVolts=0,jackVolts=0;};
namespace capricorn_detail {
template<unsigned N> struct Prepared {
    std::array<std::array<float,N>,N> A{};std::array<float,N> B{},K{},C{},H{},state{};
    float D=0,Ky=0,Ud=0,resistance=0;
    float source(float x) const noexcept {float y=Ud*x;for(unsigned k=0;k<N;++k)y+=H[k]*state[k];return y;}
    float step(float x,float drop=0) noexcept {
        float y=D*x-Ky*drop;for(unsigned k=0;k<N;++k)y+=C[k]*state[k];
        std::array<float,N> next{};for(unsigned k=0;k<N;++k){next[k]=B[k]*x-K[k]*drop;for(unsigned j=0;j<N;++j)next[k]+=A[k][j]*state[j];}state=next;return y;
    }
};
struct RC {float g=0,z=0;float low(float x) noexcept {const float v=(x-z)*g,y=z+v;z=y+v;return y;}float high(float x) noexcept {return x-low(x);}};
}
class Capricorn {
public:
    static constexpr unsigned kKnots=257,kTaps=97;
    // Stopped owner only; rejected preparation preserves the existing instance.
    bool prepare(float rate,CapricornControls c={},unsigned factor=4) noexcept;
    void reset() noexcept;void stop() noexcept {running_=false;}
    float process(float inputModelVolts) noexcept; // jackV/2, input containment +/-0.5V
    float transistorCurve(float x) const noexcept;
    float diodeCurve(float sourceVolts) const noexcept;
    const std::array<double,3>& bias() const noexcept {return bias_;}
    const CapricornProbe& probe() const noexcept {return probe_;}
    float diodeResistance() const noexcept {return post_.resistance;}
    uint32_t failures() const noexcept {return failures_;}uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t inputClamps() const noexcept {return clamps_;}uint32_t tableClamps() const noexcept {return tableClamps_;}
private:
    struct Point {float y=0,F=0;};Point qValue(float) const noexcept;float sample(float) noexcept;
    float coefficient(unsigned n) const noexcept {return fir_[n<=48?n:96-n];}
    std::array<float,kKnots> qx_{},qy_{},qs_{},qF_{},dx_{},dy_{},ds_{};
    std::array<float,49> fir_{};std::array<float,25> up_{};std::array<float,97> down_{};
    capricorn_detail::Prepared<2> front_{};capricorn_detail::Prepared<4> post_{};
    capricorn_detail::RC input_{},coupling_{},gain_{},amplifier_{},output1_{},output2_{};
    std::array<double,3> bias_{};CapricornProbe probe_{};
    float inverseGain_=1,extraGain_=0,previous_=0,previousArea_=0;
    unsigned factor_=4,upIndex_=0,downIndex_=0;uint32_t failures_=0,invalid_=0,clamps_=0,tableClamps_=0;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Capricorn)<=10240,"SF018 standalone budget, not runtime admission");
}
