// SF-019 host-only plate WAV audition. Uses unchanged Soundforge lab_wav.h.
#include "parable_plate.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace parable::dsp;
namespace fs=std::filesystem;
struct Run {lab::Wave out; PlateProbe probe;double micros=0;};
Run render(const lab::Wave& input, PlateControls c){
    Plate plate;lab::require(plate.prepare(input.rate,c),"Plate prepare failed");
    Run r;r.out.rate=input.rate;
    const std::size_t frames=std::min<std::size_t>(input.samples.size(),input.rate*8);
    r.out.samples.assign(frames+input.rate*2,0.f);
    auto t=std::chrono::steady_clock::now();
    for(std::size_t i=0;i<r.out.samples.size();++i){
        const float inputSample=i<frames?input.samples[i]*0.8f:0.f;
        r.out.samples[i]=plate.process(inputSample);
    }
    auto e=std::chrono::steady_clock::now();r.micros=std::chrono::duration<double,std::micro>(e-t).count()/r.out.samples.size();
    r.probe=plate.probe();lab::require(plate.nonfiniteOutputs()==0,"Nonfinite output");return r;
}
void persist(const fs::path& dir,const char* name,const lab::Wave& w,float gain=1){
    lab::Wave out=w;for(auto &x:out.samples)x*=gain;
    lab::write(dir/name,out);
    const auto s=soundforge::stats(out.samples);
    std::cout<<name<<" rms="<<s.rms<<" peak="<<s.peak<<" gain="<<gain<<"\n";
}
int main(int argc,char** argv){
    if(argc!=3){std::cerr<<"usage: plate_lab INPUT_MONO_PCM16.wav NEW_OUTPUT_DIR\n";return 2;}
    try{
        const fs::path input=fs::absolute(argv[1]),dir=fs::absolute(argv[2]);
        lab::require(!fs::exists(dir),"Output directory must be new");
        fs::create_directories(dir);
        auto wav=lab::read(input);
        lab::require(wav.rate==44100||wav.rate==48000||wav.rate==96000,"44.1/48/96k only");
        const auto good=render(wav,{});
        PlateControls faulty;faulty.damagedMount=true;
        const auto bad=render(wav,faulty);
        lab::Wave dry;dry.rate=wav.rate;dry.samples.assign(good.out.samples.size(),0);
        for(std::size_t i=0;i<std::min<std::size_t>(wav.samples.size(),wav.rate*8);++i)dry.samples[i]=.8f*wav.samples[i];
        persist(dir,"00_dry_raw.wav",dry);
        persist(dir,"01_healthy_raw.wav",good.out);
        persist(dir,"02_mount_contact_raw.wav",bad.out);
        const auto gs=soundforge::stats(good.out.samples),bs=soundforge::stats(bad.out.samples);
        const double maxTarget=soundforge::rmsTarget(std::array<std::vector<float>,2>{good.out.samples,bad.out.samples},.06,.86);
        persist(dir,"03_healthy_matched.wav",good.out,float(soundforge::rmsGain(gs,maxTarget)));
        persist(dir,"04_mount_contact_matched.wav",bad.out,float(soundforge::rmsGain(bs,maxTarget)));
        std::cout<<std::setprecision(8)<<"healthy_contacts="<<good.probe.contacts<<" fault_contacts="<<bad.probe.contacts
        <<" healthy_us_frame="<<good.micros<<" fault_us_frame="<<bad.micros<<" buffer_B_about=146600\n";
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
