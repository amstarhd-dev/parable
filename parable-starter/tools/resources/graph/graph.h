#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <new>
#include <type_traits>
#include "capricorn_loc.h"
#include "muff_reduced.h"
#include "fits.h"
#include "ts808_controls.h"
#include "parable_rat.h"
#include "parable_scorpio.h"
#include "parable_rotary.h"
#include "parable_coupled_rooms.h"
#include "hall_half.h"
#include "modes.h"

namespace graph {
enum Effect {Empty,Muff,Loc,Rat,TS,Centaur,CMOS,Germanium,Blender,FZ1,Saturation,Scorpio,Tape,Spring,Hall,Rooms,Rotary};
using Screamer=TunedTS808<1,false,1>;
struct SlotConfig {
    uint8_t key,effect,bypass;
    uint16_t p[7]; // integers in the hardware catalog, with dB offset by 30
};
struct Patch {SlotConfig slots[4]; uint8_t volume,tag;};
inline float bound(float x){return std::clamp(x,-1.f,1.f);}
inline float gain(unsigned n){return std::pow(10.f,(int(n)-30)*.05f);}
inline float blend(float x,float y,float mix){return x+(y-x)*mix;}
struct TapeLine {
    float buf[57600]{}, record=0,play=0,delay=0;
    unsigned w=0;
    float sample(float x,const float* k){
        delay+=(k[0]-delay)*.00024f;
        float pos=w-delay;if(pos<0)pos+=57600;
        unsigned i=unsigned(pos);
        float head=blend(buf[i],buf[(i+1)%57600],pos-i);
        play+=(head-play)*k[4];record+=(x-record)*k[3];
        float y=std::clamp(k[2]*record+k[1]*play,-3.f,3.f);
        float sq=y*y;
        buf[(w+720)%57600]*=k[6];
        buf[w]=blend(buf[w],y*(27+sq)/(27+9*sq),.6f);
        if(++w==57600)w=0;
        return blend(x,play,k[5]);
    }
};
struct SpringBank {
    float re[48]{},im[48]{},delay[48]{};unsigned w=0;
    float sample(float x,const float* k){
        float u=delay[w],wet=0;delay[w]=x;if(++w==48)w=0;
        for(unsigned i=0;i<48;++i){
            const auto* c=modes[i];float r=re[i],v=im[i];
            // First-order radius correction for the near-unit measured poles.
            float a=c[0]*k[3+i],b=c[1]*k[3+i];
            re[i]=a*r-b*v+c[2]*u;im[i]=b*r+a*v+c[3]*u;wet+=re[i];
        }
        return blend(x,2.5f*k[2]*wet,k[1]);
    }
};
struct HallTank {
    hall_half::Core core;
    float lp[3]{},smooth[3]{},target=0;unsigned phase=0;
    float sample(float x,float mix){
        // Two cascaded three-pole filters around the 12 kHz wet tank.
        float v=x;for(auto& y:lp){y+=.33f*(v-y);v=y;}
        if(++phase==4){phase=0;float l=0,r=0;float* in[]={&v,&v};float* out[]={&l,&r};
            core.process(in,out,1);target=.5f*(l+r);}
        v=target;for(auto& y:smooth){y+=.39f*(v-y);v=y;}
        return blend(x,v,mix);
    }
};
struct Room {parable::soundforge::CoupledRooms core;float mem[13000];};
using Rodent=parable::dsp::Rat;
using Transformer=parable::dsp::Scorpio;
using Rotor=parable::dsp::Rotary;
using MuffUnit=parable::dsp::ReducedMuff;
struct Unit {
    // Keep frequently accessed states in MCU SRAM and long delays in SDRAM.
    std::aligned_union_t<0,Rodent,Transformer,Rotor,MuffUnit,CapricornLoc,fitted::Drive,Screamer,SpringBank> fast;
    void* slow=nullptr;
    template<class T>T& get(){return *std::launder(reinterpret_cast<T*>(&fast));}
    template<class T>T& memory(){return *std::launder(static_cast<T*>(slow));}
    bool prepare(const SlotConfig& s){
        const auto* p=s.p;
        switch(s.effect){
            case Muff:new(&fast) MuffUnit(p[4]);break;
            case Loc:new(&fast) CapricornLoc(p[0]*.01f,p[1]);break;
            case Rat:{auto* r=new(&fast) Rodent;return r->configure(48000,{p[0]*.01f,p[1]*.01f,p[2]*.01f},{},parable::dsp::RatRate::Reference1x);}
            case TS:new(&fast) Screamer;break;
            case Scorpio:return (new(&fast) Transformer)->prepare(48000,{.75f,.35f,.8f,bool(p[2])},1);
            case Tape:(new(slow) TapeLine)->delay=p[0]*48.f;break;
            case Spring:new(&fast) SpringBank;break;
            case Hall:new(slow) HallTank;break;
            case Rooms:{auto* r=new(slow) Room;return r->core.prepare(48000,r->mem,13000,{.85f,.32f,false});}
            case Rotary:{auto* r=new(&fast) Rotor;r->interval=32;return r->prepare(48000,{bool(p[2]),p[1]*.01f});}
            case Empty:break;
            default:{
                const float (*tables[])[22]={fitted::klon6,fitted::cmos6,fitted::germanium6,fitted::blender6,fitted::fz16,fitted::tape_saturation6};
                auto* fit=new(&fast) fitted::Drive(tables[s.effect-Centaur]);fit->delay=s.effect==Saturation?176:48;
            }
        }
        return true;
    }
};
using Bulk=std::aligned_union_t<0,TapeLine,Room,HallTank>;
struct Voice {
    Unit* unit=nullptr; SlotConfig config{};float k[51]{};
    void coefficients(){
        const auto* p=config.p;for(unsigned i=0;i<7;++i)k[i]=p[i]*.01f;
        switch(config.effect){
            case Muff:k[3]=gain(p[3]+(p[4]?p[5]:0));break;
            case Scorpio:k[0]=gain(p[0]);break;
            case Tape:k[0]=p[0]*48.f;k[2]=p[2]*.05f;k[3]=1-std::exp(-.0001308997f*p[3]);k[4]=1-std::exp(-.0001308997f*p[4]);k[6]=p[6]?.85f:0;break;
            case Spring:for(unsigned i=0;i<48;++i){auto* c=modes[i];k[3+i]=std::pow(std::hypot(c[0],c[1]),100.f/p[0]-1);}break;
            default:if(config.effect>=Centaur && config.effect<=Saturation){k[0]=gain(p[0]);k[1]=gain(p[1]);}
        }
    }
    void apply(){
        const auto* p=config.p;
        switch(config.effect){
            case Muff:unit->get<MuffUnit>().setControls(k[0],k[1],k[2]);break;
            case Loc:unit->get<CapricornLoc>().setRegister(k[0]);unit->get<CapricornLoc>().setLeaky(p[1]);break;
            case Rat:unit->get<Rodent>().setControls({k[0],k[1],k[2]});break;
            case TS:unit->get<Screamer>().setControls(k[0],k[1],k[2]);break;
            case Hall:{auto& h=unit->memory<HallTank>().core;h.regeneration=.000293f*(.92f+.08f*k[0]);h.focusing=p[2]?.98f:0.f;break;}
            case Rooms:unit->memory<Room>().core.setControls({k[0],k[1],bool(p[2])});break;
            case Rotary:unit->get<Rotor>().controls(p[2],k[1]);break;
        }
    }
    float sample(float x){
        if(config.bypass)return x;
        switch(config.effect){
            case Empty:return x;
            case Muff:return bound(k[3]*unit->get<MuffUnit>().process(x));
            case Loc:return unit->get<CapricornLoc>().process(x);
            case Rat:return unit->get<Rodent>().process(x);
            case TS:return bound(3*unit->get<Screamer>().process(x));
            case Scorpio:return bound(k[1]*unit->get<Transformer>().process(k[0]*x));
            case Tape:return unit->memory<TapeLine>().sample(x,k);
            case Spring:return unit->get<SpringBank>().sample(x,k);
            case Hall:return unit->memory<HallTank>().sample(x,k[1]);
            case Rooms:return unit->memory<Room>().core.process(x).mono;
            case Rotary:return unit->get<Rotor>().process(x,k[0]);
            default:return bound(k[1]*unit->get<fitted::Drive>().process(bound(k[0]*x)));
        }
    }
};
}
