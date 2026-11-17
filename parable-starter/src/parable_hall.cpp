#include "parable_hall.h"
#include <algorithm>
#include <cmath>
namespace parable::dsp {
bool Hall::prepare(float rate,HallSettings s) noexcept {
    if ((rate!=44100 && rate!=48000 && rate!=96000) ||
        !std::isfinite(s.decay) || s.decay<0 || s.decay>1 ||
        !std::isfinite(s.wet) || s.wet<0 || s.wet>1 ||
        !std::isfinite(s.focusing) || s.focusing<0 || s.focusing>1) return false;
    settings_=s; core_.rate=rate;
    core_.regeneration=.000293*(.92+.08*double(s.decay));
    core_.focusing=s.focusing; core_.reset(); hits_=0; prepared_=true; return true;
}
void Hall::reset() noexcept {core_.reset();hits_=0;}
void Hall::process(const float* input,float* left,float* right) noexcept {
    for(size_t i=0;i<blockSize;++i) if(!prepared_ || !std::isfinite(input[i])) {
        std::fill_n(left,blockSize,0);std::fill_n(right,blockSize,0);return;
    }
    float clean[blockSize];
    for(size_t i=0;i<blockSize;++i) clean[i]=std::clamp(input[i],-1.f,1.f);
    float* ins[]={clean,clean};float* outs[]={left,right};
    core_.process(ins,outs,blockSize);
    for(size_t i=0;i<blockSize;++i) {
        left[i]=clean[i]*(1-settings_.wet)+settings_.wet*left[i];
        right[i]=clean[i]*(1-settings_.wet)+settings_.wet*right[i];
        for(float* out:{left+i,right+i}) {
            if(!std::isfinite(*out)){*out=0;++hits_;}
            else if(std::abs(*out)>.98f){*out=std::clamp(*out,-.98f,.98f);++hits_;}
        }
    }
}
}
