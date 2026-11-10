#pragma once
// CMOS-specific host fixtures/measurements. No RAT DSP/library dependency.
#include "parable_cmos.h"
#include "cmos_device.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <sstream>
#include <complex>

namespace cmoslab {
using namespace parable::dsp;
constexpr double pi=3.14159265358979323846;
inline CmosCircuit variation(const std::string& name) {
    CmosCircuit c;
    if(name=="healthy")return c;
    if(name=="coupling_small")c.interstageFarads=330e-12;
    else if(name=="feedback_low")c.secondFeedbackOhms=100e3;
    else throw std::invalid_argument("Unknown CMOS variation: "+name);
    return c;
}
inline std::vector<std::string> fields(const std::string& row) {
    std::vector<std::string> out;std::istringstream line(row);std::string field;
    while(std::getline(line,field,','))out.push_back(field);
    return out;
}
struct Errors {double dcPeak=0,transientRms=0,transientPeak=0;unsigned dcPoints=0,transientPoints=0;};
inline Errors references() {
    Errors e;std::ifstream dc(std::string(CMOS_FIXTURES)+"/inverter-dc.csv");lab::require(bool(dc),"Missing CMOS DC fixture");
    std::string row;std::getline(dc,row);
    while(std::getline(dc,row)){
        const auto f=fields(row);lab::require(f.size()==3,"Invalid CMOS DC row");
        const double load=std::stod(f[0]),gate=std::stod(f[1]),expected=std::stod(f[2]);
        double lo=0,hi=9;
        for(int i=0;i<64;++i){const double mid=(lo+hi)/2;
            if(cmos_detail::inverter(gate,mid,9).value+load*mid>0)hi=mid;else lo=mid;}
        e.dcPeak=std::max(e.dcPeak,std::abs((lo+hi)/2-expected));++e.dcPoints;
    }
    std::ifstream transient(std::string(CMOS_FIXTURES)+"/circuit-transient.csv");lab::require(bool(transient),"Missing CMOS transient fixture");
    std::getline(transient,row);std::string current;Cmos model;unsigned sample=0;double sum=0;
    while(std::getline(transient,row)){
        const auto f=fields(row);lab::require(f.size()==9,"Invalid CMOS transient row");
        if(f[0]!=current){current=f[0];sample=0;
            const bool healthy=current=="healthy_low"||current=="healthy_mid"||current=="healthy_high";
            const auto c=variation(healthy?"healthy":current);
            lab::require(model.configure(96000,{std::stof(f[1]),.5f},c,CmosRate::Reference1x),"CMOS reference config rejected");}
        const unsigned target=static_cast<unsigned>(std::stoul(f[2]));
        for(;sample<=target;++sample)model.process(static_cast<float>(.1*std::sin(2*pi*997*sample/96000)));
        const auto n=model.lastNodes();const double actual[]={n.gate1,n.output1,n.gate2,n.output2,n.jackVolts};
        lab::require(model.failedSolves()==0,"Reference signal used numerical fallback");
        for(int i=0;i<5;++i){const double error=std::abs(actual[i]-std::stod(f[i+4]));
            sum+=error*error;e.transientPeak=std::max(e.transientPeak,error);++e.transientPoints;}
    }
    lab::require(e.dcPoints==362&&e.transientPoints==4800,"Incomplete CMOS references");
    e.transientRms=std::sqrt(sum/e.transientPoints);
    lab::require(e.dcPeak<1e-5,"CMOS DC differs from independent SPICE");
    lab::require(e.transientRms<.005&&e.transientPeak<.05,"CMOS waveform exceeds reduced-circuit discrepancy gate");
    return e;
}
// Direct coherent-bin projection, not a cloned FFT. Host only. Samples are
// retained so raw mean/RMS/peak and broadband THD+N reuse Soundforge statistics.
inline double amplitude(const std::vector<float>& signal,unsigned bin) {
    std::complex<double> total=0;
    for(std::size_t i=0;i<signal.size();++i)total+=double(signal[i])*std::polar(1.,-2*pi*bin*i/signal.size());
    return 2*std::abs(total)/signal.size();
}
struct Spectrum {double fundamental=0,thdnDb=0,secondDb=0,mean=0,peak=0,aliasDb=0;unsigned maxIterations=0;};
inline Spectrum spectrum(const CmosCircuit& circuit,CmosControls controls,unsigned bin,double input,CmosRate mode=CmosRate::Oversampled4x) {
    constexpr unsigned count=4096;Cmos model;lab::require(model.configure(48000,controls,circuit,mode),"CMOS measurement config rejected");
    Spectrum m;std::vector<float> signal(count);
    for(unsigned i=0;i<9*count;++i){const float y=model.process(static_cast<float>(input*std::sin(2*pi*bin*i/count)));
        m.maxIterations=std::max(m.maxIterations,model.lastNodes().iterations);
        if(i>=8*count)signal[i-8*count]=y;}
    lab::require(model.failedSolves()==0,"Measured signal used numerical fallback");
    const auto raw=soundforge::stats(signal);m.fundamental=amplitude(signal,bin);m.mean=raw.mean;m.peak=raw.peak;
    const double fundamentalPower=.5*m.fundamental*m.fundamental;
    const double residual=std::max(0.,raw.rms*raw.rms-raw.mean*raw.mean-fundamentalPower);
    m.thdnDb=10*std::log10(std::max(residual/fundamentalPower,1e-30));
    m.secondDb=20*std::log10(std::max(amplitude(signal,2*bin)/m.fundamental,1e-15));
    const unsigned folded=(3*bin)%count;
    m.aliasDb=20*std::log10(std::max(amplitude(signal,folded)/m.fundamental,1e-15));
    return m;
}
} // namespace cmoslab
