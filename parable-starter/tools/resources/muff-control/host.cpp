#include "muff_reduced.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc,char** argv){
    if(argc!=5&&argc!=6)return 2;
    const float sustain=std::strtof(argv[1],nullptr);
    const float tone=std::strtof(argv[2],nullptr);
    const float volume=std::strtof(argv[3],nullptr);
    parable::dsp::ReducedMuff effect(std::atoi(argv[4])!=0);
    effect.setControls(sustain,tone,volume);
    for(unsigned i=0;i<48000;++i)effect.process(0);
    float input;
    unsigned n=0;
    while(std::fread(&input,sizeof(input),1,stdin)==1){
        if(argc==6 && n%48==0){
            const float ramp=(n%48000)/48000.f;
            effect.setControls(ramp,1-ramp,ramp);
        }
        const float output=effect.process(input);
        if(!std::isfinite(output)||std::fwrite(&output,sizeof(output),1,stdout)!=1)return 4;
        ++n;
    }
}
