#include "parable_muff.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
using namespace parable::dsp;
int main(int argc,char** argv){
    if(argc!=5)return 2;
    const MuffConfig cfg{atoi(argv[4])!=0,float(atof(argv[1])),float(atof(argv[2])),float(atof(argv[3]))};
    auto model=std::make_unique<Muff>();
    if(!model->prepare(48000,cfg))return 3;
    float x;
    while(fread(&x,sizeof(x),1,stdin)==1){
        float jack=model->process(x);
        float result[2]={float(model->nodes()[Muff::C2]-model->biasNodes()[Muff::C2]),jack};
        if(fwrite(result,sizeof(float),2,stdout)!=2)return 4;
    }
    return model->failures()?5:0;
}
