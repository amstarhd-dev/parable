#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Host prototype: 48 kHz, drive 1, tone and level 0.5, healthy diode pair.
#ifndef TS808_STAGES
#define TS808_STAGES 1
#endif
class ReducedTS808 {
    static_assert(TS808_STAGES>=1 && TS808_STAGES<=3);

    static constexpr double fs = 48000, inner = (1<<TS808_STAGES)*fs;
    static constexpr double rf = 551000, rg = 4700, cf = 51e-12;
    static constexpr float maxCurrent = 0.002f;
    static constexpr size_t cells = 2048;
    std::array<float,cells+1> volts{};
    std::array<float,31> fir{};
    using Phases = std::array<float,16>;
    struct History { Phases input{}, even{}, odd{}; };
    std::array<History, TS808_STAGES> stages{};
    float in1=0, in2=0, src1=0, src2=0, low=0, vfb=0;
    float z1=0, z2=0, out1=0, out2=0;
    float b0,b1,b2,a1,a2;
    static float hp(float x,float& lastX,float& lastY,float pole) {
        float y=pole*(lastY+x-lastX);
        lastX=x; lastY=y; return y;
    }
    static void push(Phases& history,float x) {
        std::move_backward(history.begin(),history.end()-1,history.end());
        history.front()=x;
    }
    float evenSum(const Phases& history) const {
        float sum=0;
        for(size_t i=0;i<8;++i)
            sum+=(history[i]+history[15-i])*fir[2*i];
        return sum;
    }
    float clip(float x) {
        constexpr float alpha=1.f/(1.f+inner*rg*47e-9);
        low+=alpha*(x-low);
        float current=(x-low)/rg+inner*cf*vfb;
        float u=cells*std::sqrt(std::min(std::abs(current)/maxCurrent,1.f));
        size_t i=std::min(static_cast<size_t>(u),cells-1);
        vfb=std::copysign(volts[i]+(u-i)*(volts[i+1]-volts[i]),current);
        return x+vfb;
    }
    template<size_t stage>
    float resample(float x) {
        if constexpr(stage == TS808_STAGES) {
            return clip(x);
        } else {
            auto& h=stages[stage];
            push(h.input,x);
            push(h.even,resample<stage+1>(2*evenSum(h.input)));
            float y=evenSum(h.even)+fir[15]*h.odd[7];
            push(h.odd,resample<stage+1>(2*fir[15]*h.input[7]));
            return y;
        }
    }
public:
    ReducedTS808() {
        for(size_t i=0;i<=cells;++i) {
            double u=double(i)/cells, target=maxCurrent*u*u, lo=0, hi=1;
            for(int n=0;n<48;++n) {
                double mid=(lo+hi)*.5;
                double cur=(1/rf+inner*cf)*mid+2*2.52e-9*std::sinh(mid/.045293);
                if(cur<target) lo=mid; else hi=mid;
            }
            volts[i]=(lo+hi)*.5;
        }
        constexpr double pi=3.141592653589793;
        double sum=0;
        for(int n=0;n<31;++n) {
            int k=n-15;
            fir[n]=(k==0?.5:std::sin(.5*pi*k)/(pi*k))*(.42-.5*std::cos(2*pi*n/30)+.08*std::cos(4*pi*n/30));
            sum+=fir[n];
        }
        for(float& h:fir) h/=sum;
        // Ideal op-amp reduction of the 1k / 10k / 220nF tone network at noon.
        double tau=5220*220e-9, k=2*fs;
        double d0=1.1, d1=d0*tau+1000*220e-9*1.5, d2=1000*220e-9*tau;
        double n1=tau+500*220e-9, den=d0+k*d1+k*k*d2;
        b0=(1+k*n1)/den; b1=2/den; b2=(1-k*n1)/den;
        a1=(2*d0-2*k*k*d2)/den; a2=(d0-k*d1+k*k*d2)/den;
    }
    float process(float x) {
        x=hp(x,in1,in2,.9974f);
        x=hp(x,src1,src2,.99795f);
        float y=resample<0>(x);
        float t=b0*y+z1; z1=b1*y-a1*t+z2; z2=b2*y-a2*t;
        return hp(t,out1,out2,.99955f)*.113f;
    }
};
