// Host-only complete-processor sampling/cost study, not a device microbenchmark.
#include "parable_blender.h"
#include "lab_wav.h"
#include <array>
#include <chrono>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <iostream>
using parable::dsp::Blender;
using parable::dsp::BlenderConfig;
using parable::dsp::BlenderRate;
constexpr unsigned size=2048;
constexpr double pi=3.14159265358979323846;
static unsigned bin(unsigned rate,unsigned kind){return unsigned((kind==0?1760:kind==1?3520:8000)*double(size)/rate)|1u;}
static float excitation(unsigned rate,unsigned kind,double amplitude,double phaseFrame) {
    double x=0;
    if(kind<3)x=std::sin(2*pi*bin(rate,kind)*phaseFrame/size);
    else{double scale=0;for(unsigned b:{37u,47u,59u})for(unsigned h=1;h<=5;++h){x+=std::sin(2*pi*b*h*phaseFrame/size)/(h*h);scale+=1./(h*h);}x/=scale;}
    return float(amplitude*x);
}
static void writeFloat(std::ostream& out,float x){static_assert(sizeof(float)==4,"binary32 study");uint32_t bits=0;std::memcpy(&bits,&x,4);for(unsigned j=0;j<4;++j)out.put(char(bits>>(8*j)));}
int main(int argc,char** argv)try {
    lab::require(argc==2,"Require output directory");const std::filesystem::path out=argv[1];std::filesystem::create_directories(out);std::ofstream log(out/"runs.csv");log<<std::setprecision(16);
    log<<"id,rate_hz,fault,input_peak_bound_V,kind,bin,factor,direct_reference,warm_base_frames,frames,cpu_us_per_frame,wall_us_per_frame,mean_evaluations_per_internal,maximum_evaluations,failures,input_clamps,output_clamps,state_bytes\n";
    unsigned id=0;bool clean=true;
    for(unsigned rate:{44100u,48000u,96000u})for(bool fault:{false,true})for(double amplitude:{.05,.2})for(unsigned kind=0;kind<4;++kind)for(unsigned factor:{1u,2u,4u,8u,16u,32u}){
        const bool reference=factor>8;Blender m;BlenderConfig c;c.d1Open=fault;lab::require(m.prepare(float(rate*(reference?factor:1)),c,reference?BlenderRate::Direct:BlenderRate(factor)),"Sampling preparation failed");
        const unsigned warm=unsigned(std::ceil(.5*rate/size))*size,delay=factor==1||reference?0:48,total=(warm+size+delay)*(reference?factor:1);
        // Host excitation generation is outside the processor timing interval.
        std::vector<float> input(size*(reference?factor:1));for(unsigned n=0;n<input.size();++n)input[n]=excitation(rate,kind,amplitude,double(n)/(reference?factor:1));
        std::vector<float> y;y.reserve(size*(reference?factor:1));const auto startCpu=std::clock();const auto start=std::chrono::steady_clock::now();
        for(unsigned n=0;n<total;++n){const auto v=m.process(input[n%input.size()]);if(n>=(warm+delay)*(reference?factor:1))y.push_back(v);}
        const double cpuUs=1e6*double(std::clock()-startCpu)/CLOCKS_PER_SEC,wallUs=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        const bool ok=!m.failures()&&!m.inputClamps()&&!m.outputClamps()&&!m.invalidInputs();clean=clean&&ok;
        const std::string stem=std::to_string(id);std::ofstream raw(out/(stem+".f32"),std::ios::binary);for(float v:y)writeFloat(raw,v);raw.flush();lab::require(bool(raw),"Study output write failed");
        const double baseFrames=double(total)/(reference?factor:1);log<<id<<','<<rate<<','<<fault<<','<<amplitude<<','<<kind<<','<<(kind<3?bin(rate,kind):0)<<','<<factor<<','<<reference<<','<<warm<<','<<y.size()<<','<<cpuUs/baseFrames<<','<<wallUs/baseFrames<<','<<double(m.evaluations())/m.solves()<<','<<m.maximumEvaluations()<<','<<m.failures()<<','<<m.inputClamps()<<','<<m.outputClamps()<<','<<sizeof(m)<<'\n';log.flush();lab::require(bool(log),"Study receipt write failed");std::cout<<"sampling "<<id++<<" rate "<<rate<<" fault "<<fault<<" factor "<<factor<<" clean "<<ok<<std::endl;
    }
    return clean?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
