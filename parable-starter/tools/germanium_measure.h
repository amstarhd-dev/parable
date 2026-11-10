#pragma once
// Host-only, circuit-specific measurements; shared PCM/statistics remain unchanged.
#include "parable_germanium.h"
#include "germanium_device.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <complex>
#include <sstream>
namespace germaniumlab {
using namespace parable::dsp;
constexpr double pi=3.14159265358979323846;
inline GermaniumCircuit variation(const std::string& name){
    GermaniumCircuit c;
    if(name=="healthy")return c;
    if(name=="q1_leaky")c.q1.bcLeakageAmps*=10;
    else if(name=="feedback_wrong")c.feedbackOhms=10e3;
    else throw std::invalid_argument("Unknown germanium variation: "+name);
    return c;
}
inline std::vector<std::string> fields(const std::string& row){
    std::vector<std::string> out;std::istringstream in(row);std::string s;
    while(std::getline(in,s,','))out.push_back(s);
    return out;
}
struct Errors {double deviceRelative=0,biasPeak=0,nodeRms=0,nodePeak=0,jackRms=0,jackPeak=0,inputRms=0;};
inline std::array<double,6> values(const GermaniumNodes& n){return {n.base1,n.collector1,n.emitter2,n.collector2,n.tap,n.jack};}
inline GermaniumCircuit referenceConfig(const std::vector<std::string>& f,bool transient){
    // Explicit case allowlist; fixture labels are never treated as unknown healthy.
    lab::require(f[0]=="healthy_low"||f[0]=="healthy_mid"||f[0]=="healthy_high"||f[0]=="source_high"||f[0]=="weak_input"
        ||f[0]=="q1_leaky"||f[0]=="feedback_wrong","Unknown germanium reference case");
    GermaniumCircuit c;c.sourceOhms=std::stod(f[2]);c.q1.bcLeakageAmps=std::stod(f[transient?4:3]);c.feedbackOhms=std::stod(f[transient?5:4]);return c;
}
inline Errors references(unsigned referenceRate=192000,bool enforce=true){
    lab::require(referenceRate==96000||referenceRate==192000,"Unsupported reference grid");
    Errors e;std::string row;unsigned deviceCount=0,biasCount=0,nodeCount=0,jackCount=0;
    std::ifstream device(std::string(GERMANIUM_FIXTURES)+"/device-current.csv");lab::require(bool(device),"Missing independent Ge device fixture");std::getline(device,row);
    while(std::getline(device,row)){const auto f=fields(row);lab::require(f.size()==7,"Bad device row");
        GermaniumDevice p;p.forwardGain=std::stod(f[1]);p.bcLeakageAmps=std::stod(f[2]);
        const auto m=germanium_detail::device(std::stod(f[3]),std::stod(f[4]),p);lab::require(m.valid,"Invalid device reference domain");
        const double expected[]={std::stod(f[5]),std::stod(f[6])},actual[]={m.base,m.collector};
        for(int i=0;i<2;++i){const double error=std::abs(actual[i]-expected[i]);e.deviceRelative=std::max(e.deviceRelative,error/(1e-9+std::abs(expected[i])));
            lab::require(error<1e-10+1e-6*std::abs(expected[i]),"Ge current differs from independent SPICE");}++deviceCount;}
    std::ifstream bias(std::string(GERMANIUM_FIXTURES)+"/bias.csv");lab::require(bool(bias),"Missing Ge bias fixture");std::getline(bias,row);
    while(std::getline(bias,row)){const auto f=fields(row);lab::require(f.size()==11,"Bad Ge bias row");Germanium m;
        lab::require(m.configure(48000,{std::stof(f[1]),.5f},referenceConfig(f,false)),"Bias setup rejected");
        const auto actual=values(m.bias());for(unsigned i=0;i<6;++i)e.biasPeak=std::max(e.biasPeak,std::abs(actual[i]-std::stod(f[i+5])));++biasCount;}
    std::ifstream transient(std::string(GERMANIUM_FIXTURES)+"/transient.csv");lab::require(bool(transient),"Missing Ge transient fixture");std::getline(transient,row);
    std::string current;Germanium m;unsigned sample=0;double sum=0,jackSum=0,inputSum=0;
    while(std::getline(transient,row)){const auto f=fields(row);lab::require(f.size()==14,"Bad Ge transient row");
        if(f[0]!=current){current=f[0];sample=0;lab::require(m.configure(float(referenceRate),{std::stof(f[1]),.5f},referenceConfig(f,true),GermaniumRate::Reference1x),"Transient setup rejected");}
        const unsigned target=static_cast<unsigned>(std::stoul(f[6]))*(referenceRate/96000);const double level=std::stod(f[3]);
        for(;sample<=target;++sample)m.process(static_cast<float>(level*std::sin(2*pi*997*sample/referenceRate)));
        lab::require(m.failedSolves()==0&&m.containedOutputs()==0,"Independent case used fallback/clipping");const auto actual=values(m.lastNodes());
        for(unsigned i=0;i<5;++i){const double error=actual[i]-std::stod(f[i+7]);sum+=error*error;e.nodePeak=std::max(e.nodePeak,std::abs(error));++nodeCount;}
        const double jack=actual[5]-std::stod(f[12]),input=m.lastNodes().inputAmps-std::stod(f[13]);
        jackSum+=jack*jack;inputSum+=input*input;e.jackPeak=std::max(e.jackPeak,std::abs(jack));++jackCount;}
    lab::require(deviceCount==123&&biasCount==7&&jackCount==1344&&nodeCount==6720,"Incomplete independent Ge fixtures");
    e.nodeRms=std::sqrt(sum/nodeCount);e.jackRms=std::sqrt(jackSum/jackCount);e.inputRms=std::sqrt(inputSum/jackCount);
    lab::require(e.biasPeak<1e-5,"Ge bias differs from independent solve");
    lab::require(!enforce||(e.nodeRms<.02&&e.nodePeak<.2&&e.jackRms<.0015&&e.jackPeak<.01&&e.inputRms<1e-6),
        ("Ge transient gate: nodes RMS/peak="+std::to_string(e.nodeRms)+"/"+std::to_string(e.nodePeak)
        +" jack RMS/peak="+std::to_string(e.jackRms)+"/"+std::to_string(e.jackPeak)+" input RMS="+std::to_string(e.inputRms)).c_str());
    return e;
}
// Five-line coherent projection idiom adapted from SF002, no copied FFT/parser.
inline std::complex<double> projection(const std::vector<float>& s,unsigned bin){
    std::complex<double> total=0;
    for(std::size_t i=0;i<s.size();++i)total+=double(s[i])*std::polar(1.,-2*pi*bin*i/s.size());
    return 2.*total/double(s.size());
}
struct Spectrum {double fundamental=0,thdn=0,second=0,foldedThird=0,mean=0,peak=0,zinReal=0,zinImag=0;unsigned maxEvaluations=0;};
inline Spectrum spectrum(const GermaniumCircuit& c,GermaniumControls controls,unsigned bin,double level,GermaniumRate mode=GermaniumRate::Oversampled4x){
    constexpr unsigned n=4096;Germanium m;lab::require(m.configure(48000,controls,c,mode),"Ge spectrum setup rejected");
    std::vector<float> signal(n),source(n),current(n);Spectrum out;
    for(unsigned i=0;i<n*5;++i){const float x=static_cast<float>(level*std::sin(2*pi*bin*i/n)),y=m.process(x);
        if(i>=n*4){signal[i-n*4]=y;source[i-n*4]=x;current[i-n*4]=static_cast<float>(m.lastNodes().inputAmps);}}
    lab::require(m.failedSolves()==0&&m.containedOutputs()==0,
        ("Spectrum containment: fuzz="+std::to_string(controls.fuzz)+" source="+std::to_string(c.sourceOhms)
        +" bin="+std::to_string(bin)+" level="+std::to_string(level)+" mode="+std::to_string(int(mode))
        +" failed="+std::to_string(m.failedSolves())+" clipped="+std::to_string(m.containedOutputs())).c_str());
    const auto stats=soundforge::stats(signal);out.fundamental=std::abs(projection(signal,bin));out.mean=stats.mean;out.peak=stats.peak;
    const double residual=std::max(0.,stats.rms*stats.rms-stats.mean*stats.mean-.5*out.fundamental*out.fundamental);
    out.thdn=10*std::log10(std::max(residual/(.5*out.fundamental*out.fundamental),1e-30));
    out.second=20*std::log10(std::max(std::abs(projection(signal,2*bin))/out.fundamental,1e-15));
    out.foldedThird=20*std::log10(std::max(std::abs(projection(signal,(3*bin)%n))/out.fundamental,1e-15));
    // LastNodes is internal-phase current. Input impedance therefore uses1x only;
    // no uncorrected FIR group delay is presented as complex port impedance.
    if(mode==GermaniumRate::Reference1x){const auto z=projection(source,bin)/projection(current,bin)-c.sourceOhms;out.zinReal=z.real();out.zinImag=z.imag();}
    out.maxEvaluations=m.maximumLoopEvaluations();return out;
}
} // namespace germaniumlab
