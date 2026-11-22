#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

// 48 kHz mono. Original behavioral design for the Mars drive slot.
class CapricornLoc {
    float registerTarget_=1, register_=1, charge_=0, inputDC_=0, outputDC_=0;
    bool leaky_=false;
    float up_[9]{},down_[17]{};
    unsigned upPos_=0,downPos_=0;
    // Symmetric 17-tap Kaiser halfband at 96 kHz. Odd offsets from center only.
    static constexpr float h_[9]={-2.898498961e-19f,-3.056490721e-03f,3.188365547e-18f,1.946494363e-02f,-9.411783740e-18f,-7.126149452e-02f,1.638316841e-17f,3.048911786e-01f,4.999237261e-01f};
    static float bend(float x) noexcept {
        // Smooth saturation, asymmetric rails, zero maps exactly to zero.
        const float scale=x<0?.69f:1.f;
        const float v=std::min(std::abs(x)/scale,3.f), vv=v*v;
        return std::copysign(scale*v*(27.f+vv)/(27.f+9.f*vv),x);
    }
    float cell(float x) noexcept {
        const float r=register_;
        const float driven=x*(2.f+46.f*r*r);
        const float rectified=std::abs(driven);
        // A leaky timing capacitor discharges between string cycles.
        const float release=leaky_?.00346f:.0000374f;
        charge_+=(rectified>charge_?.084f:release)*(rectified-charge_);
        const float ceiling=.22f;
        const float gain=ceiling/std::max(ceiling,charge_);
        // The mic preamp precedes gain reduction. The limited node then drives
        // a second biased stage. Harder playing changes the harmonic balance.
        const float pre=bend(driven*.78f)/.78f;
        const float crushed=pre*gain*(2.f+10.f*r);
        return .84f*bend(crushed);
    }
public:
    explicit CapricornLoc(float amount=1,bool leaky=false) noexcept
      : registerTarget_(std::clamp(amount,0.f,1.f)),register_(registerTarget_),leaky_(leaky) {}
    void setLeaky(bool enabled) noexcept { leaky_ = enabled; }
    void setRegister(float amount) noexcept {
        if(std::isfinite(amount))registerTarget_=std::clamp(amount,0.f,1.f);
    }
    float process(float x) noexcept {
        if(!std::isfinite(x))x=0;
        x=std::clamp(x,-1.f,1.f);
        register_+=.00208f*(registerTarget_-register_);
        inputDC_+=.00457f*(x-inputDC_);
        up_[upPos_]=x-inputDC_;
        for(unsigned phase=0;phase<2;++phase){
            float v=0;
            for(unsigned tap=phase;tap<17;tap+=2)
                v+=h_[std::min(tap,16-tap)]*up_[(upPos_+9-tap/2)%9];
            down_[downPos_]=cell(2*v);
            downPos_=(downPos_+1)%17;
        }
        upPos_=(upPos_+1)%9;
        float y=0;
        for(unsigned t=0;t<17;++t)y+=h_[std::min(t,16-t)]*down_[(downPos_+16-t)%17];
        outputDC_+=.00261f*(y-outputDC_);
        return std::clamp(y-outputDC_,-1.f,1.f);
    }
};
