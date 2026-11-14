// Independent scientific reference, never linked to native DSP. Deliberately
// includes NO native material/stimulus headers or interpolation/event code.
// Long-double, resolved Karlqvist carrier at uniform spatial/time steps. Shared
// physical assumptions are documented: this is numerical, not measured validation.
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
using Real=long double;
constexpr Real pi=3.141592653589793238462643383279502884L;
constexpr Real speed=.381L,span=80e-6L/speed,omega=2*pi*100000;
Real source(const std::string& kind,Real t,Real a) {
    if(t<0)return 0;
    if(kind=="dc")return a;
    if(kind=="tone")return a*std::sin(2*pi*1000*t);
    if(kind=="hf")return a*std::sin(2*pi*10000*t);
    if(kind=="two")return a*(std::sin(2*pi*700*t)+std::sin(2*pi*1100*t))/2;
    if(kind=="chord")return a*(std::sin(2*pi*196*t)+std::sin(2*pi*246.94165L*t)+std::sin(2*pi*293.66477L*t))/3;
    const Real age=std::fmod(t,.22L);
    return a*(1-std::exp(-age*3000))*std::exp(-age*12)*(std::sin(2*pi*196*t)+.25L*std::sin(2*pi*392*t))/1.25L;
}
struct Reference {
    std::array<Real,8> radius{},weight{};
    std::vector<Real> tau,field,bSin,bCos,audio;
    explicit Reference(unsigned samplesPerCycle) {
        Real total=0;for(unsigned i=0;i<8;++i){radius[i]=15000+3000*i;weight[i]=std::exp(-.5L*std::pow((radius[i]-25500)/6000,2));total+=weight[i];}
        for(auto& w:weight)w/=total;
        const unsigned count=unsigned(std::ceil(span*100000*samplesPerCycle));
        for(unsigned j=0;j<=count;++j){const Real t=-span+span*j/count,x=speed*(t+span/2);
            const Real k=(std::atan((x+3.5e-6L)/8.3e-6L)-std::atan((x-3.5e-6L)/8.3e-6L))/pi;
            tau.push_back(t);field.push_back(k*180/7e-6L);bSin.push_back(std::sin(omega*t));bCos.push_back(std::cos(omega*t));}
        audio.resize(tau.size());
    }
    void drive(const std::string& kind,Real time,Real amplitude){for(std::size_t j=0;j<tau.size();++j)audio[j]=.010L*source(kind,time+tau[j],amplitude);}
    Real parcel(Real phase,Real bias) const {
        std::array<Real,8> boundary{};const Real s=std::sin(phase),c=std::cos(phase);
        for(std::size_t j=0;j<tau.size();++j){const Real h=field[j]*(audio[j]+.012L*bias*(bSin[j]*c+bCos[j]*s));
            // Independently expressed projection of the positive relay population.
            for(unsigned i=0;i<8;++i)boundary[i]=std::min(h+radius[i],std::max(h-radius[i],boundary[i]));}
        // Exterior of the declared finite fringe is H=0, so report remanence,
        // not the in-field magnetization at the last spatial sample.
        Real result=0;for(unsigned i=0;i<8;++i){boundary[i]=std::clamp(boundary[i],-radius[i],radius[i]);result+=weight[i]*std::erf(boundary[i]/(12000*std::sqrt(2.L)));}return result;
    }
    Real average(unsigned phases,Real bias) const {Real result=0;for(unsigned q=0;q<phases;++q)result+=parcel(2*pi*(q+.5L)/phases,bias)/phases;return result;}
};
std::ofstream file(const std::filesystem::path& p){std::ofstream f(p);if(!f)throw std::runtime_error("Cannot create reference output");f<<std::setprecision(20);return f;}
void fixtures(const std::filesystem::path& out) {
    std::filesystem::create_directories(out);auto f=file(out/"material.csv");f<<"kind,amplitude_V,base_rate_hz,weak,internal_frame,M_reference,M_refined\n";
    Reference reference(128),refined(256);
    for(unsigned rate:{44100u,48000u,96000u})for(bool weak:{false,true})for(const std::string kind:{"tone","hf","two","attacks","chord","dc"})for(Real a:{.02L,.2L,.8L}) {
        // Startup/history and steady portions, sampled sparsely; native tests
        // advance every intervening4x sample. No native-generated goldens.
        const unsigned end=rate*4/125; //8ms
        for(unsigned n=0;n<=end;n+=std::max(1u,end/40)) {
            const Real t=Real(n)/(4*rate);reference.drive(kind,t,a);const Real m=reference.average(32,weak?.5L:1.L);
            Real fine=m;if(n%(std::max(1u,end/40)*8)==0){refined.drive(kind,t,a);fine=refined.average(64,weak?.5L:1.L);}
            f<<kind<<','<<a<<','<<rate<<','<<weak<<','<<n<<','<<m<<','<<fine<<'\n';
        }
    }
    auto c=file(out/"candidates.csv");c<<"bias_ratio,input_V,phases,carrier_steps_per_cycle,remanence\n";
    for(Real bias:{1.L,.5L,.25L})for(Real a:{0.L,.01L,.05L,.1L,.2L,.4L,.8L,1.6L})for(unsigned phases:{4u,8u,16u,32u,64u}){
        reference.drive("dc",.01L,a);c<<bias<<','<<a<<','<<phases<<",128,"<<reference.average(phases,bias)<<'\n';}
    auto r=file(out/"refinement.csv");r<<"bias_ratio,kind,input_V,time_s,phases,carrier_steps_per_cycle,remanence\n";
    for(unsigned steps:{64u,128u,256u}){Reference model(steps);for(Real bias:{1.L,.5L,.25L})for(const std::string kind:{"tone","hf","two","attacks"})for(unsigned q:{8u,16u,32u,64u})for(unsigned i=1;i<=4;++i){
        const Real t=.00013L+i*.00027L;model.drive(kind,t,.8L);r<<bias<<','<<kind<<",.8,"<<t<<','<<q<<','<<steps<<','<<model.average(q,bias)<<'\n';}}
    std::cout<<"Independent long-double material/candidate/refinement fixtures complete\n";
}
void carrier(const std::filesystem::path& path,unsigned outputRate,unsigned steps,const std::string& kind,Real amplitude,Real bias) {
    Reference model(steps);auto f=file(path);f<<"frame,time_s,full_carrier_M\n";
    //20ms after20ms warmup, coherent with1k/10k and100k. This computes an
    // ACTUAL continuous carrier phase, not independent phase-averaged parcels.
    const unsigned count=outputRate/50;
    for(unsigned n=0;n<count;++n){const Real t=.02L+Real(n)/outputRate;model.drive(kind,t,amplitude);f<<n<<','<<t<<','<<model.parcel(omega*t,bias)<<'\n';}
}
}
int main(int argc,char** argv)try {
    if(argc==3&&std::string(argv[1])=="--fixtures"){fixtures(argv[2]);return 0;}
    if(argc==9&&std::string(argv[1])=="--carrier"){
        const auto rate=std::stoul(argv[3]),steps=std::stoul(argv[4]);const auto amplitude=std::stold(argv[6]),bias=std::stold(argv[7]);
        if(std::string(argv[8])!="offline")throw std::runtime_error("Require offline label");
        if((rate!=1536000&&rate!=3072000)||(steps!=64&&steps!=128&&steps!=256)||
           (std::string(argv[5])!="tone"&&std::string(argv[5])!="hf")||
           !std::isfinite(amplitude)||amplitude<=0||amplitude>2||!std::isfinite(bias)||bias<=0||bias>1)
            throw std::runtime_error("Unsupported carrier probe (see reproducible evidence recipe)");
        carrier(argv[2],unsigned(rate),unsigned(steps),argv[5],amplitude,bias);
        return 0;}
    throw std::runtime_error("Use --fixtures DIR or --carrier CSV RATE STEPS tone|hf AMP BIAS_RATIO offline");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
