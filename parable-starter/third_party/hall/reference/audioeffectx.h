#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
using VstInt32=int32_t; using audioMasterCallback=void*;
constexpr int kVstMaxProgNameLen=24,kVstMaxParamStrLen=8,kVstMaxProductStrLen=64,kVstMaxVendorStrLen=64;
enum VstPlugCategory {kPlugCategEffect};
struct AudioEffect {virtual ~AudioEffect()=default;};
struct AudioEffectX:AudioEffect {double rate=48000;AudioEffectX(void*,int,int){};void setSampleRate(double r){rate=r;}double getSampleRate() const{return rate;}void setNumInputs(int){}void setNumOutputs(int){}void setUniqueID(unsigned long){}void canProcessReplacing(){}void canDoubleReplacing(){}void programsAreChunks(bool){}};
inline void vst_strncpy(char* d,const char*s,int n){std::strncpy(d,s,n);d[n]=0;}
inline void float2string(float x,char*d,int n){std::snprintf(d,n,"%.3f",double(x));}
