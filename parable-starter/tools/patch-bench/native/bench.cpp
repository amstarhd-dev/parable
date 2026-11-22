#include <algorithm>
#include <new>

#include "hall_core.h"
#include "parable_coupled_rooms.h"
#include "parable_rat.h"
#include "parable_scorpio.h"
#include "ts808_controls.h"

using parable::dsp::Rat;
using parable::dsp::Scorpio;
using parable::hall_detail::Core;
using parable::soundforge::CoupledRooms;
using Screamer = TunedTS808<1,false,1>;

namespace {
constexpr unsigned slotCount=4;
struct NativeSlot {
    Rat rat;
    Scorpio scorpio;
    Core hall;
    CoupledRooms rooms;
    alignas(Screamer) unsigned char screamerStorage[sizeof(Screamer)];
    float roomMemory[13000]{};
    unsigned effect=0;
    bool fault=false;
    float input=1, level=1, targetInput=1, targetLevel=1, targetDecay=1, targetFocus=0;
    Screamer* screamer() noexcept {return std::launder(reinterpret_cast<Screamer*>(screamerStorage));}
};
NativeSlot slots[slotCount];
}

extern "C" int bench_prepare(unsigned slot,unsigned effect) {
    if(slot>=slotCount || effect<1 || effect>5)return 0;
    auto& s=slots[slot];
    const unsigned old=s.effect;
    s.effect=effect;
    s.fault=false;
    s.input=s.level=s.targetInput=s.targetLevel=s.targetDecay=1;
    s.targetFocus=0;
    if(old==2)s.screamer()->~Screamer();
    switch(effect){
        case 1:
            return s.rat.configure(48000,{.72f,.50f,.75f});
        case 2:
            new(s.screamerStorage) Screamer();
            return 1;
        case 3:
            return s.scorpio.prepare(48000,{.75f,.35f,.8f,false},4);
        case 4:
            s.hall.regeneration=.000293f;
            s.hall.focusing=0;
            s.hall.reset();
            return 1;
        case 5:
            return s.rooms.prepare(48000,s.roomMemory,13000,{.85f,.35f,false});
    }
    return 0;
}

extern "C" void bench_controls(unsigned slot,float a,float b,float c,int fault) {
    if(slot>=slotCount)return;
    auto& s=slots[slot];
    switch(s.effect){
        case 1:
            s.rat.setControls({a,b,c});
            break;
        case 2:
            s.screamer()->setControls(a,b,c);
            break;
        case 3:
            if (s.fault!=(fault!=0)) {
                const bool changed=s.scorpio.prepare(48000,{.75f,.35f,.8f,fault!=0},4);
                if (changed) s.fault=fault!=0;
            }
            s.targetInput=a;
            s.targetLevel=c;
            break;
        case 4:
            s.targetDecay=a;
            s.targetLevel=c;
            s.targetFocus=fault?.98f:0;
            break;
        case 5:
            s.rooms.setControls({a,c,fault!=0});
            break;
    }
}

extern "C" float bench_sample(unsigned slot,float x) {
    if (slot>=slotCount)return x;
    auto& s=slots[slot];
    switch(s.effect){
        case 1: return s.rat.process(x)*4;
        case 2: return s.screamer()->process(x)*3;
        case 3:
            s.input+=.002f*(s.targetInput-s.input);
            s.level+=.002f*(s.targetLevel-s.level);
            return s.scorpio.process(s.input*x)*s.level;
        case 4: {
            s.level+=.002f*(s.targetLevel-s.level);
            const float regen=.000293f*(.92f+.08f*s.targetDecay);
            s.hall.regeneration+=.002f*(regen-s.hall.regeneration);
            s.hall.focusing+=.002f*(s.targetFocus-s.hall.focusing);
            float l=0,r=0;
            float* ins[2]={&x,&x};
            float* outs[2]={&l,&r};
            s.hall.process(ins,outs,1);
            return x+s.level*(.5f*(l+r)-x);
        }
        case 5:
            return s.rooms.process(x).mono;
    }
    return x;
}
