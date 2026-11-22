#pragma once
#include "graph.h"
namespace graph {
constexpr unsigned bytes=70;
using Wire=std::array<uint8_t,bytes>;
inline Wire encode(const Patch& patch){
    Wire v{};unsigned pos=0;
    for(auto& s:patch.slots){
        v[pos++]=s.key;v[pos++]=s.effect;v[pos++]=s.bypass;
        for(auto n:s.p){v[pos++]=n&255;v[pos++]=n>>8;}
    }
    v[pos++]=patch.volume;v[pos]=patch.tag;return v;
}
inline bool decode(const Wire& v,Patch& patch){
    unsigned pos=0,used=0;
    for(auto& s:patch.slots){
        s.key=v[pos++];s.effect=v[pos++];s.bypass=v[pos++];
        if(s.key>=4 || s.effect>Rotary || s.bypass>1 || (used&(1u<<s.key)))return false;
        used|=1u<<s.key;
        for(auto& n:s.p){n=v[pos]|(uint16_t(v[pos+1])<<8);pos+=2;}
        const auto* p=s.p;
        if(s.effect>=Centaur && s.effect<=Saturation){if(p[0]>60||p[1]>60)return false;continue;}
        switch(s.effect){
            case Tape:if(p[0]<20||p[0]>1130||p[1]>80||p[2]<5||p[2]>160||p[3]<200||p[3]>10000||p[4]<200||p[4]>6000||p[5]>100||p[6]>1)return false;break;
            case Spring:if(p[0]<25||p[0]>125||p[1]>100||p[2]>200)return false;break;
            case Muff:if(p[0]>100||p[1]>100||p[2]>100||p[3]>54||p[4]>1||p[5]>18)return false;break;
            case Scorpio:if(p[0]>42||p[1]>200||p[2]>1)return false;break;
            case Loc:if(p[0]>100||p[1]>1)return false;break;
            case Hall:case Rooms:case Rotary:if(p[0]>100||p[1]>100||p[2]>1)return false;break;
            case Rat:case TS:if(p[0]>100||p[1]>100||p[2]>100)return false;break;
            default:break;
        }
    }
    patch.volume=v[pos++];patch.tag=v[pos];return patch.volume<=100;
}
}
