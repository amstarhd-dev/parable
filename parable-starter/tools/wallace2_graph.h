#pragma once
#include "wallace_graph.h"
#include "wallace_cabinet.h"
namespace wallace2 {
enum class Placement { PreCab, PostCab };
struct Config {
    float clean=.50f,muff=.75f,rotary=.18f,tape=.12f,hall=.12f;
    bool ducking=false,cleanDisabledControl=false;
    Placement placement=Placement::PreCab;
};
struct Block {
    // Weighted signal contributions. wet is beforecab for PreCab, final for PostCab.
    std::array<float,64> clean{},fuzz{},wet{},out{},duck{};
};
class Graph {
public:
    bool prepare(const float* ir,size_t taps,Config c={}) {
        if(!std::isfinite(c.clean)||c.clean<0||c.clean>2||!std::isfinite(c.muff)||c.muff<0||c.muff>2)return false;
        if((c.cleanDisabledControl&&(c.clean!=0||c.placement!=Placement::PreCab))||(!c.cleanDisabledControl&&c.clean<.05f))return false;
        prepared_=false;c_=c;
        wallace::RoutingConfig returns;returns.core=returns.clean=0;
        returns.rotary=c.rotary;returns.tape=c.tape;returns.hall=c.hall;
        returns.ducking=c.ducking;returns.duckDb=2.5f;
        if(!returns_.prepare(returns)||!cab_.prepare(48000,ir,taps))return false;
        if(!muff_.prepare(48000,{false,.65f,.5f,.65f})||!rotary_.prepare(48000,{false,1}))return false;
        parable::dsp::TapeConfig t;t.loopSeconds=1.2f;t.delaySeconds=.3f;t.feedback=.32f;t.drive=2.4f;t.mix=1;
        medium_.resize(parable::dsp::Tape::requiredCells(48000,t));
        if(!tape_.prepare(48000,t,medium_.data(),medium_.size())||!hall_.prepare(48000,{1,1,0}))return false;
        delay_.fill(0);position_=0;prepared_=true;return true;
    }
    bool process(const float* input,Block& b) noexcept {
        if(!prepared_||!input){b={};return false;}
        for(size_t j=0;j<64;++j)if(!std::isfinite(input[j])||std::abs(input[j])>1){b={};return false;}
        for(size_t j=0;j<64;++j){
            const float clean=delay_[position_];delay_[position_]=input[j];if(++position_==48)position_=0;
            b.clean[j]=c_.clean*clean;b.fuzz[j]=c_.muff*muff_.process(input[j]);
            bus_[j]=b.clean[j]+b.fuzz[j];send_[j]=clean;
        }
        if(c_.placement==Placement::PostCab){if(!cab_.process(bus_.data(),cabBus_.data()))return false;send_=cabBus_;}
        for(size_t j=0;j<64;++j){rot_[j]=rotary_.process(send_[j],.7f);tap_[j]=tape_.process(send_[j]);}
        hall_.process(send_.data(),left_.data(),right_.data());
        for(size_t j=0;j<64;++j){
            const auto r=returns_.process(input[j],0,rot_[j],tap_[j],(left_[j]+right_[j])*.5f);
            b.wet[j]=r.wet;b.duck[j]=r.gain;
            if(c_.placement==Placement::PreCab)bus_[j]+=r.wet;
            else b.out[j]=cabBus_[j]+r.wet; // Cabinet dry bus NEVER replaced by wet returns.
        }
        if(c_.placement==Placement::PreCab&&!cab_.process(bus_.data(),b.out.data()))return false;
        return true;
    }
    bool healthy() const noexcept {return !muff_.failures()&&!muff_.outputClamps()&&!muff_.inputClamps()&&!muff_.invalidInputs()&&!rotary_.containmentEvents()&&!hall_.containmentHits();}
    size_t fixedBytes() const noexcept {return sizeof(*this)+medium_.capacity()*sizeof(float);} // excludes cab heap
private:
    parable::dsp::Muff muff_;parable::dsp::Rotary rotary_;parable::dsp::Tape tape_;parable::dsp::Hall hall_;
    std::vector<float> medium_;Cabinet cab_;wallace::Routing returns_;Config c_{};
    std::array<float,48> delay_{};size_t position_=0;bool prepared_=false;
    std::array<float,64> bus_{},cabBus_{},send_{},rot_{},tap_{},left_{},right_{};
};
}
