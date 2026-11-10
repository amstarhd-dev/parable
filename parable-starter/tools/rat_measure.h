#pragma once
// Host analysis only. Independent SPICE fixtures exercise actual shared stages.
#include "parable_rat.h"
#include "lab_wav.h"
#include <complex>
#include <sstream>
#include <iostream>
#include <stdexcept>
namespace ratlab {
constexpr double pi=3.14159265358979323846;
using namespace parable::dsp;
inline RatCircuit variation(const std::string& name) {
    RatCircuit c;
    if(name=="healthy")return c;
    if(name=="diode_open")c.positiveDiodeConnected=false;
    else if(name=="ruetz")c.trebleLegConnected=false;
    else if(name=="compensation")c.feedbackCompFarads=1e-7;
    else throw std::invalid_argument("Unknown RAT variation: "+name);
    return c;
}
inline std::vector<std::string> fields(std::string row) {
    std::vector<std::string> out;std::istringstream line(row);std::string f;
    while(std::getline(line,f,','))out.push_back(f);
    return out;
}
inline std::complex<double> response(const rat_detail::Filter& f,double radians) {
    std::complex<double> b=0,a=0;for(unsigned i=0;i<=f.order;++i){
        const auto z=std::polar(1.,-radians*i);b+=f.b[i]*z;a+=f.a[i]*z;}
    return b/a;
}
struct ReferenceErrors {double maxGainDb=0,maxPhaseDegrees=0,sameFrequencyGainDb=0,transientRms=0,transientPeak=0;};
inline ReferenceErrors spiceReferences() {
    ReferenceErrors error;std::ifstream ac(std::string(RAT_FIXTURES)+"/preamp-ac.csv");lab::require(bool(ac),"Missing SPICE AC fixture");
    std::string row;std::getline(ac,row);unsigned count=0;
    while(std::getline(ac,row)){
        const auto r=fields(row);lab::require(r.size()==5,"Bad AC reference row");
        const double frequency=std::stod(r[2]);const std::complex<double> expected{std::stod(r[3]),std::stod(r[4])};
        rat_detail::Filter input,gain;const auto c=variation(r[0]);
        lab::require(rat_detail::preparePreamp(input,gain,192000,{std::stof(r[1]),.5f,.7f},c),"Preamp coefficient failure");
        // Bilinear frequency mapping removes warping from this analog comparison.
        const double w=2*std::atan(pi*frequency/192000);
        const auto actual=response(input,w)*response(gain,w);
        error.maxGainDb=std::max(error.maxGainDb,std::abs(20*std::log10(std::abs(actual/expected))));
        error.maxPhaseDegrees=std::max(error.maxPhaseDegrees,std::abs(std::arg(actual/expected))*180/pi);++count;
        const auto direct=response(input,2*pi*frequency/192000)*response(gain,2*pi*frequency/192000);
        error.sameFrequencyGainDb=std::max(error.sameFrequencyGainDb,std::abs(20*std::log10(std::abs(direct/expected))));
    }
    lab::require(count>=300,"Incomplete AC reference grid");
    std::ifstream trans(std::string(RAT_FIXTURES)+"/clip-transient.csv");lab::require(bool(trans),"Missing SPICE transient fixture");
    std::getline(trans,row);std::string current;rat_detail::ClipStage stage;int sample=0;double sum=0;unsigned points=0;
    while(std::getline(trans,row)){
        const auto r=fields(row);lab::require(r.size()==6,"Bad transient reference row");
        if(r[0]!=current){current=r[0];sample=0;const float filter=current=="filter_dark"?1.f:.5f;stage.prepare(192000,{.5f,filter,.7f},variation(current=="filter_dark"?"healthy":current));}
        const int target=std::stoi(r[1]);double jack=0;
        for(;sample<=target;++sample)jack=stage.process(3*std::sin(2*pi*997*sample/192000));
        const double expected[]={std::stod(r[3]),std::stod(r[4]),std::stod(r[5])};
        const double actual[]={stage.clip,stage.tone,jack};
        for(int i=0;i<3;++i){const double delta=std::abs(actual[i]-expected[i]);sum+=delta*delta;error.transientPeak=std::max(error.transientPeak,delta);++points;}
    }
    error.transientRms=std::sqrt(sum/points);lab::require(points>=1700,"Incomplete transient fixtures");
    lab::require(error.maxGainDb<.05&&error.maxPhaseDegrees<.5,"Preamp disagrees with independent SPICE AC");
    lab::require(error.sameFrequencyGainDb<1,"Uncorrected bilinear frequency error exceeds 1 dB");
    lab::require(error.transientRms<.01&&error.transientPeak<.05,"Clipping/filter node discrepancy exceeds reduced-circuit gate");
    return error;
}
inline void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
constexpr unsigned fftSize=8192;
struct Spectrum {double fundamental=0,thdnDb=0,evenDb=0,dc=0,clipDc=0,peak=0,aliasDb=0;};
inline Spectrum spectrum(RatControls p,const RatCircuit& c,unsigned bin,double amplitude,RatRate mode=RatRate::Oversampled4x) {
    Rat rat;lab::require(rat.configure(48000,p,c,mode),"RAT measurement configuration rejected");
    std::vector<std::complex<double>> bins(fftSize);Spectrum m;
    for(unsigned i=0;i<5*fftSize;++i){const auto y=rat.process(static_cast<float>(amplitude*std::sin(2*pi*bin*i/fftSize)));
        if(i>=4*fftSize){bins[i-4*fftSize]=y;m.peak=std::max(m.peak,std::abs(double(y)));m.clipDc+=rat.lastNodes().clip;}}
    fft(bins);m.dc=bins[0].real()/fftSize;m.clipDc/=fftSize;
    const double fundamental=std::norm(bins[bin]);double distortion=0,alias=0;
    for(unsigned i=1;i<fftSize/2;++i){if(i!=bin)distortion+=std::norm(bins[i]);
        bool retained=false;for(unsigned h=1;h*bin<fftSize/2;++h)if(i==h*bin)retained=true;
        if(!retained)alias+=std::norm(bins[i]);}
    m.fundamental=2*std::abs(bins[bin])/fftSize;
    m.thdnDb=10*std::log10(std::max(distortion/fundamental,1e-30));
    m.aliasDb=10*std::log10(std::max(alias/fundamental,1e-30));
    m.evenDb=20*std::log10(std::max(std::abs(bins[2*bin])/std::abs(bins[bin]),1e-15));return m;
}
} // namespace ratlab
