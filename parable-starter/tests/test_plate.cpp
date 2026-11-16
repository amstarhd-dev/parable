#include "parable_plate.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(false)
using namespace parable::dsp;
int main(){
    for(float fs:{44100.f,48000.f,96000.f}){
        Plate p;CHECK(p.prepare(fs));CHECK(p.bufferBytes()>20000 && p.bufferBytes()<1200000);
        const float first=p.process(.4f);CHECK(std::isfinite(first));
        for(unsigned n=0;n<unsigned(fs*1.5f);++n){float v=p.process(0);CHECK(std::isfinite(v)&&std::abs(v)<1);}
        p.reset();CHECK(p.probe().contacts==0);
        CHECK(!p.prepare(1));CHECK(!p.prepare(fs,{2,.5f,.3f,false}));
        CHECK(!p.prepare(fs,{.5f,.5f,.3f,false}));
        CHECK(!p.prepare(fs,{.5f,15000.f,1.2f,false}));
        CHECK(p.process(NAN)==0);CHECK(p.invalidSamples()==1);
        p.stop();CHECK(p.process(.1f)==0);
    }
    int testCase=0;
    for(float amplitude:{.0001f,.04f,.30f}){
        Plate good,bad;auto c=PlateControls{};c.wet=1;
        CHECK(good.prepare(48000,c));c.damagedMount=true;CHECK(bad.prepare(48000,c));
        float diff=0,energy=0;
        for(int n=0;n<48000*2;++n){
            float x= n<48000/3 ? amplitude*std::sin(2*3.14159265358979323846*220*n/48000):0;
            float a=good.process(x),b=bad.process(x);
            CHECK(std::isfinite(a)&&std::isfinite(b));
            diff+=(a-b)*(a-b);energy+=a*a;
        }
        CHECK(good.probe().contacts==0);
        if(testCase==0)CHECK(bad.probe().contacts==0);
        else CHECK(bad.probe().contacts>100 && diff>1e-4f);
        ++testCase;
        std::cout<<"AMP "<<amplitude<<" good_contacts "<<good.probe().contacts<<" bad_contacts "<<bad.probe().contacts
        <<" difference_rms "<<std::sqrt(diff/(48000*2))<<" healthy_rms "<<std::sqrt(energy/(48000*2))
        <<" peak_mech "<<bad.probe().peakDisplacement<<" buffer "<<good.bufferBytes()<<"\n";
    }
}
