#include "wallace_routing.h"
#include <algorithm>
#include <cmath>
namespace wallace {
void Routing::Highpass::prepare(float hz) noexcept {
    const double k=std::tan(3.14159265358979323846*hz/48000.);
    const double n=1/(1+std::sqrt(2.)*k+k*k);
    b0=float(n);b1=float(-2*n);b2=b0;a1=float(2*(k*k-1)*n);
    a2=float((1-std::sqrt(2.)*k+k*k)*n);z1=z2=0;
}
float Routing::Highpass::process(float x) noexcept {
    const float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;
}
bool Routing::prepare(const RoutingConfig& c) noexcept {
    const auto range=[](float x,float lo,float hi){return std::isfinite(x)&&x>=lo&&x<=hi;};
    for(float x:{c.core,c.clean,c.rotary,c.tape,c.hall,c.sendRotary,c.sendTape,c.sendHall})if(!range(x,0,2))return false;
    if(!range(c.tapeHz,20,1000)||!range(c.hallHz,20,1000)||!range(c.attackMs,1,100)||
       !range(c.releaseMs,10,2000)||!range(c.duckDb,0,6)||!range(c.low,0,.99f)||
       !range(c.high,.001f,1)||c.high<=c.low)return false;
    config_=c;tape_.prepare(c.tapeHz);hall_.prepare(c.hallHz);cleanDelay_.fill(0);
    position_=0;envelope_=0;attack_=float(std::exp(-1/(.001*c.attackMs*48000)));
    release_=float(std::exp(-1/(.001*c.releaseMs*48000)));
    minGain_=float(std::pow(10.,-c.duckDb/20.));return true;
}
Returns Routing::process(float input,float fuzz,float rotary,float tape,float hall) noexcept {
    const float magnitude=std::abs(input);
    const float coefficient=magnitude>envelope_?attack_:release_;
    envelope_=coefficient*envelope_+(1-coefficient)*magnitude;
    const float amount=std::clamp((envelope_-config_.low)/(config_.high-config_.low),0.f,1.f);
    const float gain=config_.ducking?1-(1-minGain_)*amount:1;
    Returns r;r.envelope=envelope_;r.gain=gain;
    r.core=config_.core*fuzz;r.clean=config_.clean*cleanDelay_[position_];
    cleanDelay_[position_]=input;if(++position_==cleanDelay_.size())position_=0;
    r.rotary=config_.rotary*rotary;
    r.tape=config_.tape*gain*(config_.carving?tape_.process(tape):tape);
    r.hall=config_.hall*gain*(config_.carving?hall_.process(hall):hall);
    r.wet=r.rotary+r.tape+r.hall;r.output=r.core+r.clean+r.wet;return r;
}
}
