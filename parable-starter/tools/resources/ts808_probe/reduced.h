#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <type_traits>

// Host prototype: 48 kHz, drive 1, tone and level 0.5, healthy diode pair.
// First and second antiderivatives of the tabulated static diode map.
struct TS808FirstEntry { float v, primitive; };
struct TS808SecondEntry { float v, primitive; double second; };
template<unsigned substeps, bool fullFir, unsigned antialias>
class TunedTS808 {
    static_assert(substeps==1 || substeps==2 || substeps==4);
    static constexpr size_t stagesCount=fullFir ? (substeps==4 ? 3 : 2) : 1;
    static constexpr float inner=96000.f*substeps;
    static constexpr double fs=48000, rf=551000, rg=4700, cf=51e-12;
    static constexpr float maxCurrent=2000.f; // microamps
    static constexpr size_t cells=2048;
    using Entry=std::conditional_t<antialias==2,TS808SecondEntry,TS808FirstEntry>;
    std::array<Entry,cells+1> curve{};
    float previousCurrent=0, previousArea=0, previousDry=0, previousOver=0;
    std::array<float,31> fir{};
    using Phases = std::array<float,16>;
    struct History { Phases input{}, even{}, odd{}; };
    std::array<History, stagesCount> stages{};
    float in1=0, in2=0, src1=0, src2=0, low=0, vfb=0;
    float olderCurrent=0; double previousSecond=0, previousSlope=0;
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
        constexpr float inputG=1.e6f/rg, feedbackG=1.e6f*inner*cf;
        const float current=std::clamp(inputG*(x-low)+feedbackG*vfb,-maxCurrent,maxCurrent);
        const float mag=std::abs(current);
        const float u=cells*std::sqrt(mag/maxCurrent);
        const size_t i=std::min(static_cast<size_t>(u),cells-1);
        const float start=(float(i)*i)*(maxCurrent/(cells*cells));
        const float span=(2*i+1)*(maxCurrent/(cells*cells));
        const float dx=mag-start;
        const float voltage=curve[i].v+(dx/span)*(curve[i+1].v-curve[i].v);
        const float endpoint=std::copysign(voltage,current);
        if constexpr(antialias) {
            // Integrate the piecewise-linear diode curve across the interval.
            // Use the instantaneous endpoint for the feedback-capacitor state.
            const float area=curve[i].primitive+0.5f*dx*(curve[i].v+voltage);
            const float change=current-previousCurrent;
            float wet;
            float dry;
            if constexpr(antialias==2) {
                const double second=std::copysign(curve[i].second+dx*(double(curve[i].primitive)+dx*(2.*curve[i].v+voltage)/6.),current);
                const double slope=std::abs(change)>1.e-5f*(1+mag) ? (second-previousSecond)/change : .5*(area+previousArea);
                const float wide=current-olderCurrent;
                if(std::abs(wide)>1.e-4f*(1+mag)) wet=2*(slope-previousSlope)/wide;
                else if(std::abs(change)>1.e-3f*(1+mag)) wet=2*(area-slope)/change;
                else wet=(endpoint+vfb)*.5f;
                olderCurrent=previousCurrent; previousSecond=second; previousSlope=slope;
                dry=previousDry;
            } else {
                wet=std::abs(change)>1.e-4f*(1.f+mag) ? (area-previousArea)/change : .5f*(endpoint+vfb);
                dry=0.5f*(previousDry+x);
            }
            previousDry=x; previousArea=area; previousCurrent=current; vfb=endpoint;
            return dry+wet;
        }
        vfb=endpoint;
        return x+endpoint;
    }
    float innerStep(float x) {
        if constexpr(fullFir || substeps==1) return clip(x);
        const float increment=(x-previousOver)/substeps;
        float sum=0;
        for(unsigned k=1;k<=substeps;++k) sum+=clip(previousOver+k*increment);
        previousOver=x;
        return sum/substeps;
    }
    template<size_t stage>
    float resample(float x) {
        if constexpr(stage == stagesCount) {
            return innerStep(x);
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
    TunedTS808() {
        for(size_t i=0;i<=cells;++i) {
            double u=double(i)/cells, target=1.e-6*maxCurrent*u*u, lo=0, hi=1;
            for(int n=0;n<48;++n) {
                double mid=(lo+hi)*.5;
                double cur=(1/rf+inner*cf)*mid+2*2.52e-9*std::sinh(mid/.045293);
                if(cur<target) lo=mid; else hi=mid;
            }
            curve[i].v=(lo+hi)*.5;
            if(i) curve[i].primitive=curve[i-1].primitive+(2*i-1)*(maxCurrent/(cells*cells))*.5f*(curve[i-1].v+curve[i].v);
            if constexpr(antialias==2) {
                if(i) {
                    const double h=(2*i-1)*(maxCurrent/(cells*cells));
                    curve[i].second=curve[i-1].second+h*(double(curve[i-1].primitive)+h*(2.*curve[i-1].v+curve[i].v)/6.);
                }
            }
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
