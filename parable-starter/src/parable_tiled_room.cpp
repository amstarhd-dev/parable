// Dattorro signal flow/taps/delay interpolation adapted from Mike Jarmy dsp-lib,
// Copyright (c)2023 Mike Jarmy, MIT. Full notice and exact original in
// third_party/tiled_room. Fixed caller storage, static compact tuning and API
// are Parable adaptations; this is not proprietary PCM70/Eos code.
#include "parable_tiled_room.h"
#include <algorithm>
#include <cmath>
namespace parable::soundforge {
namespace {constexpr float size=.30f;constexpr double pi=3.14159265358979323846;}
std::array<float,13> TiledRoom::times(float rate) noexcept {
    const float r=rate/29761.f;
    std::array<float,13> t{.004f*rate,std::ceil(142*r),std::ceil(107*r),std::ceil(379*r),std::ceil(277*r)};
    constexpr unsigned base[]={672,4453,1800,3720,908,4217,2656,3163};
    for(unsigned i=0;i<8;++i)t[5+i]=std::ceil(2*base[i]*r)*(size/2);
    return t;
}
size_t TiledRoom::requiredSamples(float rate) noexcept {
    if(rate!=44100&&rate!=48000&&rate!=96000)return 0;
    size_t n=0;for(float t:times(rate))n+=size_t(std::ceil(t))+2;return n;
}
bool TiledRoom::prepare(float rate,float* storage,size_t count,TiledSettings s) noexcept {
    const size_t required=requiredSamples(rate);
    if(!required||required>kMaxSamples||!storage||count<required||!std::isfinite(s.decaySeconds)||s.decaySeconds<.25f||s.decaySeconds>1.2f||!std::isfinite(s.wet)||s.wet<0||s.wet>1)return false;
    data_=storage;used_=required;settings_=s;const auto t=times(rate);unsigned offset=0;
    for(unsigned i=0;i<13;++i){const unsigned len=unsigned(std::ceil(t[i]))+2;delays_[i]={offset,len,0,t[i]};offset+=len;}
    const float r=rate/29761.f,ratio=size/2;
    leftTaps_={266*r*ratio,2974*r*ratio,1913*r*ratio,1996*r*ratio,1990*r*ratio,187*r*ratio,1066*r*ratio};
    rightTaps_={353*r*ratio,3627*r*ratio,1228*r*ratio,2673*r*ratio,2111*r*ratio,335*r*ratio,121*r*ratio};
    // Original size control scales maximum2x tank times by size/2. Original
    // taps use base*size/2; preserve that upstream relationship explicitly.
    gain_=float(std::exp(std::log(.001)*(.30*21589/29761.)/(4*s.decaySeconds)));
    apGain_=std::clamp(gain_+.15f,.25f,.5f);
    inputB_=float(std::exp(-2*pi*10000/rate));inputA_=1-inputB_;
    dampB_=float(std::exp(-2*pi*6500/rate));dampA_=1-dampB_;
    prepared_=true;reset();return true;
}
void TiledRoom::reset() noexcept {
    if(!prepared_)return;
    std::fill_n(data_,used_,0);for(auto& d:delays_)d.write=0;
    inputState_=leftState_=rightState_=leftOut_=rightOut_=0;
}
float TiledRoom::tap(unsigned line,float frames) const noexcept {
    const auto& d=delays_[line];const int whole=int(frames);const float frac=1-(frames-whole);
    int index=int(d.write)-1-whole;if(index<0)index+=int(d.length);
    const unsigned previous=index?unsigned(index-1):d.length-1;
    const float a=data_[d.offset+previous],b=data_[d.offset+unsigned(index)];return a+(b-a)*frac;
}
void TiledRoom::push(unsigned line,float x) noexcept {auto& d=delays_[line];data_[d.offset+d.write]=x;if(++d.write==d.length)d.write=0;}
float TiledRoom::delay(unsigned line,float x) noexcept {const float y=tap(line,delays_[line].frames);push(line,x);return y;}
float TiledRoom::allpass(unsigned line,float x,float g) noexcept {const float old=tap(line,delays_[line].frames),w=x+g*old;const float y=-g*w+old;push(line,w);return y;}
TiledOutput TiledRoom::process(float input) noexcept {
    if(!prepared_||!std::isfinite(input)||std::abs(input)>1)return {};
    float sum=delay(0,input);inputState_=sum*inputA_+inputState_*inputB_;sum=inputState_;
    sum=allpass(1,sum,.75f);sum=allpass(2,sum,.75f);sum=allpass(3,sum,.625f);sum=allpass(4,sum,.625f);
    float l=sum+rightOut_*gain_,r=sum+leftOut_*gain_;
    l=allpass(5,l,-.7f);l=delay(6,l);leftState_=l*dampA_+leftState_*dampB_;
    l=allpass(7,leftState_*gain_,apGain_);leftOut_=delay(8,l);
    r=allpass(9,r,-.7f);r=delay(10,r);rightState_=r*dampA_+rightState_*dampB_;
    r=allpass(11,rightState_*gain_,apGain_);rightOut_=delay(12,r);
    const float wl=tap(10,leftTaps_[0])+tap(10,leftTaps_[1])-tap(11,leftTaps_[2])+tap(12,leftTaps_[3])-tap(6,leftTaps_[4])-tap(7,leftTaps_[5])-tap(8,leftTaps_[6]);
    const float wr=tap(6,rightTaps_[0])+tap(6,rightTaps_[1])-tap(7,rightTaps_[2])+tap(8,rightTaps_[3])-tap(10,rightTaps_[4])-tap(11,rightTaps_[5])-tap(12,rightTaps_[6]);
    TiledOutput o;o.wetLeft=.5f*wl;o.wetRight=.5f*wr;o.left=input*(1-settings_.wet)+o.wetLeft*settings_.wet;o.right=input*(1-settings_.wet)+o.wetRight*settings_.wet;o.mono=(o.left+o.right)*.5f;return o;
}
}
