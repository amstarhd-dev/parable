#include "parable_chamber.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
using parable::soundforge::Chamber;
int main(int argc,char**argv){try{
    std::filesystem::path input,output;
    for(int i=1;i<argc;++i){const std::string option=argv[i];lab::require(i+1<argc,"Missing argument value");if(option=="--input")input=argv[++i];else if(option=="--output")output=argv[++i];else throw std::runtime_error("Unknown option");}
    lab::require(!input.empty()&&!output.empty(),"Require --input and --output");auto original=lab::read(input);
    lab::require(original.rate==48000,"Listening lab requires48k input; native supports44.1/48/96k");
    const unsigned fs=original.rate;original.samples.resize(std::min(original.samples.size(),std::size_t(fs*8)));
    const double peak=soundforge::stats(original.samples).peak;lab::require(peak>0,"Silent source");const double inputGain=.15/peak;
    for(auto&x:original.samples)x=float(x*inputGain);original.samples.resize(fs*16,0);
    std::filesystem::create_directories(output);std::ofstream levels(output/"levels.csv");levels<<std::setprecision(12)<<"file,return_gain,wet,rms,peak,offline_gain,input_gain,us_per_frame,output_clamps\n";
    std::ofstream trace(output/"loop_trace.csv");trace<<std::setprecision(9)<<"mode,frame,input,microphone,speaker,applied_return\n";
    std::map<std::string,lab::Wave> waves;waves["00_dry_raw"]=original;
    auto render=[&](const std::string&name,const lab::Wave&source,float gain,float wet,bool diagnostics,bool disconnect){
        Chamber e;lab::require(e.prepare(source.rate,gain,wet),"Prepare failed");lab::Wave y{source.rate,std::vector<float>(source.samples.size())};
        lab::Wave mic=y,speaker=y,returned=y;float in[64]{},out[64]{};double elapsed=0;
        for(std::size_t n=0;n<source.samples.size();n+=64){const auto count=std::min(std::size_t(64),source.samples.size()-n);std::fill_n(in,64,0.f);std::copy_n(source.samples.data()+n,count,in);
            if(disconnect&&n==fs*8)lab::require(e.setReturnGain(0),"Gain request failed");
            const auto start=std::chrono::steady_clock::now();lab::require(e.process(in,out),"Process rejected");elapsed+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
            std::copy_n(out,count,y.samples.data()+n);
            if(diagnostics)for(std::size_t j=0;j<count;++j){mic.samples[n+j]=e.microphone()[j];speaker.samples[n+j]=e.speaker()[j];returned.samples[n+j]=e.returnSignal()[j];if((n+j)%37==0)trace<<name<<','<<n+j<<','<<in[j]<<','<<e.microphone()[j]<<','<<e.speaker()[j]<<','<<e.returnSignal()[j]<<'\n';}
        }
        const auto stats=soundforge::stats(y.samples);levels<<name<<','<<gain<<','<<wet<<','<<stats.rms<<','<<stats.peak<<",1,"<<((&source==&original)?inputGain:1.)<<','<<elapsed/source.samples.size()<<','<<e.outputClamps()<<'\n';
        lab::require(e.outputClamps()==0,"Normal render reached safety clamp");waves[name]=std::move(y);
        if(diagnostics){waves[name+"_mic"]=std::move(mic);waves[name+"_speaker"]=std::move(speaker);waves[name+"_return"]=std::move(returned);}
    };
    for(bool fault:{false,true}){
        const std::string mode=fault?"fault":"healthy";const float gain=fault?Chamber::faultGain:Chamber::healthyGain;
        render(mode+"_raw",original,gain,.65f,true,false);
        render(mode+"_wet",original,gain,1,false,false);
        lab::Wave impulse{fs,std::vector<float>(fs*8)};impulse.samples[0]=.5f;render(mode+"_impulse",impulse,gain,1,false,false);
        for(const auto&probe:{std::string("weak"),std::string("strong"),std::string("chord")}){
            lab::Wave signal{fs,std::vector<float>(fs*12)};const double amplitude=probe=="weak"?.005:.15;
            const unsigned active=probe=="chord"?fs*2:fs/2;
            for(unsigned n=0;n<active;++n){const double tone=(std::sin(6.28318530718*220*n/fs)+.5*std::sin(6.28318530718*553*n/fs))/1.5;signal.samples[n]=float(amplitude*tone*(probe=="chord"?1:std::exp(-double(n)/8000)));}
            render(mode+"_"+probe,signal,gain,1,false,false);
            if(fault&&probe=="strong")render("fault_strong_then_disconnect",signal,gain,1,false,true);
        }
    }
    lab::Wave zero{fs,std::vector<float>(fs*12)};render("fault_unexcited_zero",zero,Chamber::faultGain,1,false,false);
    const double target=soundforge::rmsTarget(std::vector<std::vector<float>>{waves["healthy_raw"].samples,waves["fault_raw"].samples},.05,.85);
    for(const auto&name:{std::string("healthy"),std::string("fault")}){auto matched=waves[name+"_raw"];const double gain=soundforge::rmsGain(soundforge::stats(matched.samples),target);for(auto&x:matched.samples)x=float(x*gain);const auto s=soundforge::stats(matched.samples);levels<<name<<"_matched,"<<(name=="fault"?Chamber::faultGain:Chamber::healthyGain)<<",.65,"<<s.rms<<','<<s.peak<<','<<gain<<','<<inputGain<<",0,0\n";waves[name+"_matched"]=std::move(matched);}
    for(const auto&entry:waves){lab::write(output/(entry.first+".wav"),entry.second);
        if(entry.first=="healthy_impulse"||entry.first=="fault_impulse"){
            std::ofstream floats(output/(entry.first+".f32"),std::ios::binary);floats.write(reinterpret_cast<const char*>(entry.second.samples.data()),entry.second.samples.size()*sizeof(float));lab::require(bool(floats),"Cannot write float impulse");
            auto audible=entry.second;for(auto&x:audible.samples)x*=64;lab::write(output/(entry.first+"_common64.wav"),audible);
        }
    }
    std::cout<<"Actual full borrowed chamber rendered "<<waves.size()+2<<" WAVs; core="<<Chamber::coreBytes()<<" wrapper="<<sizeof(Chamber)<<" B\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
