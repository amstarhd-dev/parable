#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// 48 kHz reduced TS808, healthy diode pair. Control updates run on the DSP thread.
// First and second antiderivatives of the tabulated static diode map.
struct TS808FirstEntry { float v, primitive; };
template<unsigned substeps, bool fullFir, unsigned antialias>
class TunedTS808 {
    static_assert(antialias<=1, "Live Drive uses first-order ADAA");
    static_assert(substeps==1 || substeps==2 || substeps==4);
    static constexpr size_t stagesCount=fullFir ? (substeps==4 ? 3 : 2) : 1;
    static constexpr float inner=96000.f*substeps;
    static constexpr double fs=48000, rg=4700, cf=51e-12;
    static constexpr float maxCurrent=2000.f; // microamps
    static constexpr size_t cells=2048;
    using Entry=TS808FirstEntry;
    static constexpr unsigned driveIntervals=4;
    static constexpr double minConductance=1.0/551000, maxConductance=1.0/51000;
    std::array<std::array<Entry,cells+1>,driveIntervals+1> curves{};
    unsigned driveIndex=0;
    float driveBlend=0, driveTarget=0, drivePosition=0;
    float toneTarget=.5f, tonePosition=.5f, levelTarget=.5f, levelPosition=.5f;
    float previousCurrent=0, previousArea=0, previousDry=0, previousOver=0;
    float previousDrivePosition=0;
    std::array<float,31> fir{};
    using Phases = std::array<float,16>;
    struct History { Phases input{}, even{}, odd{}; };
    std::array<History, stagesCount> stages{};
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
    Entry tableEntry(size_t j) const {
        const Entry& a=curves[driveIndex][j];
        const Entry& b=curves[driveIndex+1][j];
        return {a.v+driveBlend*(b.v-a.v),a.primitive+driveBlend*(b.primitive-a.primitive)};
    }
    Entry evaluateCurve(float mag) const {
        const float u=cells*std::sqrt(mag/maxCurrent);
        const size_t i=std::min(static_cast<size_t>(u),cells-1);
        const float start=(float(i)*i)*(maxCurrent/(cells*cells));
        const float span=(2*i+1)*(maxCurrent/(cells*cells));
        const float dx=mag-start;
        const Entry entry=tableEntry(i);
        const float voltage=entry.v+(dx/span)*(tableEntry(i+1).v-entry.v);
        return {voltage,entry.primitive+0.5f*dx*(entry.v+voltage)};
    }
    float clip(float x) {
        constexpr float alpha=1.f/(1.f+inner*rg*47e-9);
        low+=alpha*(x-low);
        constexpr float inputG=1.e6f/rg, feedbackG=1.e6f*inner*cf;
        const float current=std::clamp(inputG*(x-low)+feedbackG*vfb,-maxCurrent,maxCurrent);
        const Entry sample=evaluateCurve(std::abs(current));
        const float endpoint=std::copysign(sample.v,current);
        if constexpr(antialias) {
            // The two endpoints must use the same primitive during a Drive sweep.
            // The physically stored feedback-capacitor state still uses vfb.
            float oldEndpoint=vfb;
            if(drivePosition!=previousDrivePosition) {
                const Entry old=evaluateCurve(std::abs(previousCurrent));
                previousArea=old.primitive;
                oldEndpoint=std::copysign(old.v,previousCurrent);
            }
            const float change=current-previousCurrent;
            const float wet=std::abs(change)>1.e-4f*(1.f+std::abs(current))
                ? (sample.primitive-previousArea)/change : .5f*(endpoint+oldEndpoint);
            const float dry=.5f*(previousDry+x);
            previousDrivePosition=drivePosition;
            previousDry=x;
            previousArea=sample.primitive;
            previousCurrent=current;
            vfb=endpoint;
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
    void updateTone(float tone) {
        // Ideal-op-amp transfer of the reference tone circuit, including the
        // wiper series resistance and its position in the output feedback path.
        const double tau=(220+20000*tone*(1-tone))*220e-9, k=2*fs;
        const double d0=1.1, d1=d0*tau+1000*220e-9*(1+tone), d2=1000*220e-9*tau;
        const double n1=tau+1000*tone*220e-9, den=d0+k*d1+k*k*d2;
        b0=(1+k*n1)/den; b1=2/den; b2=(1-k*n1)/den;
        a1=(2*d0-2*k*k*d2)/den; a2=(d0-k*d1+k*k*d2)/den;
    }
public:
    TunedTS808() {
        for(unsigned bank=0;bank<=driveIntervals;++bank) {
          auto& curve=curves[bank];
          const double g=minConductance+(maxConductance-minConductance)*bank/driveIntervals;
          for(size_t i=0;i<=cells;++i) {
            double u=double(i)/cells, target=1.e-6*maxCurrent*u*u, lo=0, hi=1;
            for(int n=0;n<48;++n) {
                double mid=(lo+hi)*.5;
                double cur=(g+inner*cf)*mid+2*2.52e-9*std::sinh(mid/.045293);
                if(cur<target) lo=mid; else hi=mid;
            }
            curve[i].v=(lo+hi)*.5;
            if(i) curve[i].primitive=curve[i-1].primitive+(2*i-1)*(maxCurrent/(cells*cells))*.5f*(curve[i-1].v+curve[i].v);
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
        updateTone(.5f);
    }
    void setControls(float drive,float tone,float level) {
        if(!std::isfinite(drive)||!std::isfinite(tone)||!std::isfinite(level)) return;
        const double pot=(std::pow(20.,std::clamp(drive,0.f,1.f))-1)/19.;
        driveTarget=std::clamp(float(driveIntervals*(1/(51000+500000*pot)-minConductance)/(maxConductance-minConductance)),0.f,float(driveIntervals));
        toneTarget=std::clamp(tone,0.f,1.f);
        levelTarget=std::clamp(level,0.f,1.f);
    }
    float process(float x) {
        // Keep audio history running when a knob changes. The update is a
        // short control ramp. Recompute Tone only while that knob is moving.
        drivePosition+=.00208f*(driveTarget-drivePosition);
        driveIndex=std::min(unsigned(drivePosition),driveIntervals-1);
        driveBlend=drivePosition-driveIndex;
        if(std::abs(toneTarget-tonePosition)>1e-5f) {
            tonePosition+=.00208f*(toneTarget-tonePosition);
            updateTone(tonePosition);
        }
        levelPosition+=.00208f*(levelTarget-levelPosition);
        x=hp(x,in1,in2,.9974f);
        x=hp(x,src1,src2,.99795f);
        float y=resample<0>(x);
        float t=b0*y+z1; z1=b1*y-a1*t+z2; z2=b2*y-a2*t;
        return hp(t,out1,out2,.99955f)*(.226f*levelPosition);
    }
};
