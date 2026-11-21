#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include "weights.h"
#include "modes.h"
#include "original_rat.h"
#include "parable_rat.h"
#include "parable_scorpio.h"
#include "parable_rotary.h"
#include "parable_tape.h"
#include "parable_coupled_rooms.h"
#include "hall_core.h"
#include "hall_half.h"
#ifndef SLOW_MEMORY
#define SLOW_MEMORY
#endif
namespace workdown {
static float activate(float x) {
    float t=std::clamp(x*64.f+512.f,0.f,1024.f);
    int k=std::min(static_cast<int>(t),1023);
    return tanhTable[k]+(t-k)*(tanhTable[k+1]-tanhTable[k]);
}
struct Drive {
    const Fit* fit=nullptr;
    float lp[8]{},last=0,postState[5]{},history[176]{};
    unsigned pos=0;
    float process(float x) {
        float f[14]={x,x-last};last=x;
        for(unsigned i=0;i<4;++i){
            lp[i]+=(1-inPole[i])*(x-lp[i]);f[2+i]=lp[i];f[6+i]=x-lp[i];
        }
        for(unsigned i=0;i<2;++i){
            lp[4+i]+=(1-inPole[i])*(std::abs(x)-lp[4+i]);
            lp[6+i]+=(1-inPole[i])*(std::clamp(10*x,-1.f,1.f)-lp[6+i]);
            f[10+i]=lp[4+i];f[12+i]=lp[6+i];
        }
        float z[6]{};
        for(unsigned j=0;j<fit->width;++j){
            auto w=fit->w[j];float v=w[14];
            for(unsigned i=0;i<14;++i)v+=w[i]*f[i];
            float h=activate(v)-activate(w[14]);
            for(unsigned i=0;i<6;++i)z[i]+=w[16+i]*h;
        }
        float y=z[0];for(unsigned i=0;i<5;++i){
            postState[i]+=(1-outPole[i])*(z[1+i]-postState[i]);y+=postState[i];
        }
        float old=history[pos];history[pos]=std::clamp(y,-1.f,1.f);
        if(++pos==fit->delay)pos=0;
        return old;
    }
};
struct Spring {
    unsigned count=0,pos=0;
    float re[192]{},im[192]{},delay[48]{};
    float process(float x) {
        float u=delay[pos];delay[pos]=x;if(++pos==48)pos=0;
        float wet=0;
        for(unsigned i=0;i<count;++i) {
            const auto* c=modes[i];float a=re[i],b=im[i];
            re[i]=c[0]*a-c[1]*b+c[2]*u;
            im[i]=c[1]*a+c[0]*b+c[3]*u;
            wet+=re[i];
        }
        return .6f*x+wet;
    }
};
struct RatOriginal {
    rat1::Rat rat;
    bool init(){return rat.configure(48000,{1,.5f,.75f},{},rat1::RatRate::Reference1x);}
    float process(float x){return rat.process(x);}
};
struct Rat {
    parable::dsp::Rat rat;
    bool init(unsigned factor){rat.factor=factor;return rat.configure(48000,{1,.5f,.75f});}
    float process(float x){return rat.process(x);}
};
template<class Core,bool Half> struct Hall {
    Core core;
    float delay[64]{},previous=0,wet=0; unsigned pos=0,phase=0;
    float process(float x){
        float sum=0;
        if(!Half || phase){
            float input=Half?(x+previous)*.5f:x,left=0,right=0;
            float* ins[2]={&input,&input};float* outs[2]={&left,&right};
            core.process(ins,outs,1);
            float next=.5f*(left+right);sum=Half?.5f*(wet+next):next;wet=next;
        } else sum=wet;
        previous=x;phase^=1;
        float old=delay[pos];delay[pos]=std::clamp(.65f*x+.35f*sum,-.98f,.98f);pos=(pos+1)%64;return old;
    }
};
using HallFull=Hall<parable::hall_detail::Core,false>;
using HallHalf=Hall<hall_half::Core,true>;
static float SLOW_MEMORY roomMemory[13000],tapeMemory[57600];
struct Chain {
    parable::dsp::Scorpio drive;
    parable::dsp::Rotary mod;
    parable::dsp::Tape delay;
    parable::soundforge::CoupledRooms room;
    unsigned factor=0,pos=0;float align[24]{};
    bool init(unsigned variant) {
        factor=variant==0?4:1;mod.interval=variant==0?1:variant==1?8:32;
        parable::dsp::TapeConfig t;t.feedback=.8f;
        return drive.prepare(48000,{1,.35f,.8f,false},factor)&&mod.prepare(48000,{false,.65f})&&
          delay.prepare(48000,t,tapeMemory,57600)&&room.prepare(48000,roomMemory,13000,{1,.65f,false});
    }
    float process(float x){
        float y=drive.process(x);
        if(factor==1){float old=align[pos];align[pos]=y;pos=(pos+1)%24;y=old;}
        return room.process(delay.process(mod.process(y,.8f))).mono;
    }
};
static constexpr unsigned modelCount=32;
using SmallStorage=std::aligned_union_t<0,Drive,Spring,Rat,RatOriginal,Chain>;
using LargeStorage=std::aligned_union_t<0,HallFull,HallHalf>;
static SmallStorage small;
static LargeStorage SLOW_MEMORY large;
static float (*process)(float);
template<class T> static T* start(void* memory) {
    auto* model=new(memory) T{};
    process=[](float x){if constexpr(std::is_same_v<T,HallFull>||std::is_same_v<T,HallHalf>)
        return std::launder(reinterpret_cast<T*>(&large))->process(x);
        else return std::launder(reinterpret_cast<T*>(&small))->process(x);};
    return model;
}
static bool select(unsigned i) {
#if !defined(WORK_BANK) || WORK_BANK == 0
    if(i<21){start<Drive>(&small)->fit=&fits[i];return true;}
#endif
#if !defined(WORK_BANK) || WORK_BANK == 1
    if(i==21)return start<RatOriginal>(&small)->init();
    if(i<24)return start<Rat>(&small)->init(i==22?2:4);
    if(i<27){start<Spring>(&small)->count=48u<<(i-24);return true;}
#endif
#if !defined(WORK_BANK) || WORK_BANK == 2
    if(i==27){start<HallHalf>(&large);return true;}
    if(i==28){start<HallFull>(&large);return true;}
#endif
#if !defined(WORK_BANK) || WORK_BANK == 3
    return i<modelCount && start<Chain>(&small)->init(i-29);
#endif
    return false;

}
}
