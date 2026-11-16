#include "parable_coupled_rooms.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
using parable::soundforge::CoupledRooms;
using parable::soundforge::RoomControls;
int main(int argc, char** argv) {
    try {
        std::filesystem::path input, output;
        for (int i=1;i<argc;++i) {
            const std::string arg=argv[i]; lab::require(i+1<argc,"Missing argument value");
            if(arg=="--input") input=argv[++i];
            else if(arg=="--output") output=argv[++i];
            else throw std::runtime_error("Unknown option");
        }
        lab::require(!input.empty()&&!output.empty(),"Require --input and --output");
        auto source=lab::read(input); lab::require(CoupledRooms::requiredSamples(source.rate)>0,"Unsupported model rate");
        source.samples.resize(std::min(source.samples.size(),std::size_t(source.rate*8)));
        const auto original=soundforge::stats(source.samples);
        lab::require(original.peak>0,"Silent source");
        const double inputGain=.15/original.peak;
        for(auto& x:source.samples)x=static_cast<float>(x*inputGain);
        source.samples.resize(source.samples.size()+4*source.rate,0);
        std::filesystem::create_directories(output);
        std::ofstream receipt(output/"levels.csv");
        receipt<<std::setprecision(12)<<"file,rms,peak,matching_gain,input_gain,us_per_frame\n";
        std::vector<float> storage(CoupledRooms::requiredSamples(source.rate));
        CoupledRooms engine;
        std::map<std::string,std::vector<float>> waves;
        waves["00_dry_raw"]=source.samples;
        for(bool fault:{false,true}) {
            const std::string name=fault?"fault":"healthy";
            lab::require(engine.prepare(source.rate,storage.data(),storage.size(),{.85f,.65f,fault}),"prepare failed");
            auto& mono=waves[name+"_raw"];auto& a=waves[name+"_room_A"];auto& b=waves[name+"_room_B"];
            mono.resize(source.samples.size());a.resize(mono.size());b.resize(mono.size());
            const auto start=std::chrono::steady_clock::now();
            for(std::size_t i=0;i<mono.size();++i){const auto y=engine.process(source.samples[i]);mono[i]=y.mono;a[i]=y.a;b[i]=y.b;}
            const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/mono.size();
            const auto stats=soundforge::stats(mono);
            receipt<<name<<"_raw,"<<stats.rms<<','<<stats.peak<<",1,"<<inputGain<<','<<us<<'\n';
            auto& wet=waves[name+"_wet_only"];wet.resize(mono.size());
            for(std::size_t i=0;i<wet.size();++i)wet[i]=.5f*(a[i]+b[i]);
        }
        const double target=soundforge::rmsTarget(std::vector<std::vector<float>>{waves["healthy_raw"],waves["fault_raw"]},.05,.85);
        for(const auto& name:{"healthy","fault"}) {
            auto matched=waves[std::string(name)+"_raw"];
            const double gain=soundforge::rmsGain(soundforge::stats(matched),target);
            for(auto& x:matched)x=static_cast<float>(x*gain);
            const auto stats=soundforge::stats(matched);
            receipt<<name<<"_matched,"<<stats.rms<<','<<stats.peak<<','<<gain<<','<<inputGain<<",0\n";
            waves[std::string(name)+"_matched"]=std::move(matched);
        }
        lab::require(engine.prepare(source.rate,storage.data(),storage.size(),{0,.65f,false}),"prepare failed");
        auto& off=waves["coupling_off_B"];off.resize(source.samples.size());
        auto& offA=waves["coupling_off_A"];offA.resize(off.size());
        for(std::size_t i=0;i<off.size();++i){const auto y=engine.process(source.samples[i]);off[i]=y.b;offA[i]=y.a;}
        for(const auto& entry:waves)lab::write(output/(entry.first+".wav"),{source.rate,entry.second});
        for(bool fault:{false,true}) {
            lab::require(engine.prepare(source.rate,storage.data(),storage.size(),{.85f,1,fault}),"prepare failed");
            lab::Wave ir{source.rate,std::vector<float>(source.rate*4)};
            lab::Wave a=ir,b=ir;
            for(std::size_t i=0;i<ir.samples.size();++i){const auto y=engine.process(i==0?.5f:0);ir.samples[i]=y.mono;a.samples[i]=y.a;b.samples[i]=y.b;}
            const std::string prefix=fault?"impulse_fault":"impulse_healthy";
            lab::write(output/(prefix+".wav"),ir);lab::write(output/(prefix+"_A.wav"),a);lab::write(output/(prefix+"_B.wav"),b);
        }
        std::cout<<"Rendered actual native coupled-room guitar and impulse files; buffer "<<engine.bufferBytes()<<" B, state "<<sizeof(engine)<<" B\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
