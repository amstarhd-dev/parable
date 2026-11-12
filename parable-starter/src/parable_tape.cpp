// Circular storage/reset and one-pole/SoftClip adapted from DaisySP, MIT:
// Copyright (c) 2020 Electrosmith, Corp, Emilie Gillet.
// See third_party/tape/DaisySP-LICENSE.txt and provenance.json for exact pin.
// Three material heads and erase/partial-overwrite equations are Parable-local.
#include "parable_tape.h"
#include <cmath>
#include <limits>

namespace parable::dsp {
namespace {
bool between(float x,float lo,float hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
float softClip(float x) noexcept {
    if(x < -3.f)return -1.f;
    if(x > 3.f)return 1.f;
    return x*(27.f+x*x)/(27.f+9.f*x*x);
}
float clamp(float x) noexcept {return x < -1.f ? -1.f : x > 1.f ? 1.f : x;}
uint32_t rounded(float rate,double seconds) noexcept {
    return static_cast<uint32_t>(std::lround(double(rate)*double(seconds)));
}
}
uint32_t Tape::requiredCells(float rate,const TapeConfig& c) noexcept {
    if(!between(rate,32000,96000)||!between(c.loopSeconds,.1f,2.f)
       ||!between(c.delaySeconds,.02f,1.98f)||!between(c.eraseRetention,0,.95f)
       ||!between(c.feedback,0,.8f)||!between(c.drive,.25f,8)
       ||!between(c.recordHz,200,10000)||!between(c.replayHz,200,10000)
       ||!between(c.mix,0,1))return 0;
    const auto n=rounded(rate,c.loopSeconds),d=rounded(rate,c.delaySeconds),e=rounded(rate,kEraseToWriteSeconds);
    return n<=kMaximumCells && d+e<n ? n:0;
}
bool Tape::prepare(float rate,const TapeConfig& c,float* storage,size_t capacity) noexcept {
    const auto n=requiredCells(rate,c);
    if(!n||!storage||reinterpret_cast<uintptr_t>(storage)%alignof(float)||capacity<n)return false;
    config_=c;medium_=storage;cells_=n;delay_=rounded(rate,c.delaySeconds);erase_=rounded(rate,kEraseToWriteSeconds);
    recordCoefficient_=float(1-std::exp(-6.2831853071795864769*double(c.recordHz)/double(rate)));
    replayCoefficient_=float(1-std::exp(-6.2831853071795864769*double(c.replayHz)/double(rate)));
    reset();return true;
}
void Tape::reset() noexcept {
    for(uint32_t i=0;i<cells_;++i)medium_[i]=0;
    write_=pass_=0;record_=replay_=0;probe_={};running_=medium_!=nullptr;
}
float Tape::process(float input) noexcept {
    if(!running_)return 0;
    // Invalid input becomes silence but the physical transport still advances.
    const float x=std::isfinite(input)?clamp(input):0;
    uint32_t e=write_+erase_;if(e>=cells_)e-=cells_;
    const uint32_t p=write_>=delay_ ? write_-delay_ : write_+cells_-delay_;
    const float playback=medium_[p];
    replay_+=replayCoefficient_*(playback-replay_);
    record_+=recordCoefficient_*(x-record_);
    const float recorded=softClip(config_.drive*record_+config_.feedback*replay_);
    const float old=medium_[e];medium_[e]=config_.eraseRetention*old;
    const float prior=medium_[write_];
    const float written=(1-kWriteFraction)*prior+kWriteFraction*recorded;
    medium_[write_]=written;
    probe_={old,medium_[e],prior,recorded,written,playback,replay_,write_,e,p,pass_};
    if(++write_==cells_) {write_=0;if(pass_!=std::numeric_limits<uint32_t>::max())++pass_;}
    return (1-config_.mix)*x+config_.mix*replay_;
}
} // namespace parable::dsp
