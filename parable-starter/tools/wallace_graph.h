#pragma once
#include "parable_muff.h"
#include "parable_rotary.h"
#include "parable_tape.h"
#include "parable_hall.h"
#include "wallace_routing.h"
#include <array>
#include <cmath>
#include <vector>
namespace wallace {
enum class Mode { Serial, Static, Dynamic };
struct Block {
    std::array<float,64> fuzz{},rotary{},tape{},hall{},wet{},envelope{},gain{},out{};
};
// Literal four-processor HOST laboratory. All vector work occurs in prepare.
// Same owner; no graph edits/patch activation, no additional queued block.
class Graph {
public:
    bool prepare(Mode mode,RoutingConfig c={}) {
        prepared_=false;mode_=mode;config_=c;
        if(!mix_.prepare(c))return false;
        if(!muff_.prepare(48000,{false,.65f,.5f,.65f}))return false;
        if(!rotary_.prepare(48000,{false,mode==Mode::Serial?.35f:1.f}))return false;
        parable::dsp::TapeConfig t;t.loopSeconds=1.2f;t.delaySeconds=.3f;t.feedback=.32f;
        t.drive=2.4f;t.mix=mode==Mode::Serial?.25f:1.f;
        medium_.resize(parable::dsp::Tape::requiredCells(48000,t));
        if(!tape_.prepare(48000,t,medium_.data(),medium_.size()))return false;
        if(!hall_.prepare(48000,{1,mode==Mode::Serial?.2f:1.f,0}))return false;
        c.ducking=mode==Mode::Dynamic&&c.ducking;
        if(!mix_.prepare(c))return false;
        prepared_=true;return true;
    }
    bool process(const float* input,Block& b) noexcept {
        for(unsigned j=0;j<64;++j)if(!prepared_||!std::isfinite(input[j])){b={};return false;}
        for(unsigned j=0;j<64;++j){
            b.fuzz[j]=muff_.process(input[j]);
            const float r=rotary_.process(b.fuzz[j]*(mode_==Mode::Serial?1:config_.sendRotary),.7f);
            b.rotary[j]=mode_==Mode::Serial?rotary_.hornContribution()+rotary_.drumContribution():r;
            const float t=tape_.process(mode_==Mode::Serial?r:b.fuzz[j]*config_.sendTape);
            b.tape[j]=mode_==Mode::Serial?tape_.probe().replay:t;
            hallInput_[j]=mode_==Mode::Serial?t:b.fuzz[j]*config_.sendHall;
        }
        hall_.process(hallInput_.data(),left_.data(),right_.data());
        for(unsigned j=0;j<64;++j){
            b.hall[j]=(left_[j]+right_[j])*.5f;
            if(mode_==Mode::Serial){b.out[j]=b.hall[j];b.wet[j]=0;b.envelope[j]=0;b.gain[j]=1;}
            else {const auto r=mix_.process(input[j],b.fuzz[j],b.rotary[j],b.tape[j],b.hall[j]);
                b.out[j]=r.output;b.wet[j]=r.wet;b.envelope[j]=r.envelope;b.gain[j]=r.gain;}
        }
        return true;
    }
    bool healthy() const noexcept {return !muff_.failures()&&!muff_.outputClamps()&&!muff_.inputClamps()&&!muff_.invalidInputs()&&!rotary_.containmentEvents()&&!hall_.containmentHits();}
    size_t persistentBytes() const noexcept {return sizeof(*this)+medium_.capacity()*sizeof(float);}
    parable::dsp::Muff muff_;parable::dsp::Rotary rotary_;parable::dsp::Tape tape_;
    parable::dsp::Hall hall_;Routing mix_;std::vector<float> medium_;
private:
    std::array<float,64> hallInput_{},left_{},right_{};
    Mode mode_=Mode::Serial;RoutingConfig config_{};bool prepared_=false;
};
}
