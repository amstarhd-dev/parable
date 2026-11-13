#include "parable_ts808.h"
#include "ts808_device.h"
#include "ts808_stimulus.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <complex>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Ts808;using parable::dsp::Ts808Controls;using parable::dsp::Ts808Rate;
namespace device=parable::dsp::ts808_detail;namespace guard=activation_guard;
static std::vector<std::vector<std::string>> csv(const char* name) {
    std::ifstream f(std::string(TS808_FIXTURES)+"/"+name);CHECK(bool(f));std::string line;std::getline(f,line);
    std::vector<std::vector<std::string>> rows;
    while(std::getline(f,line)){std::stringstream in(line);std::string s;std::vector<std::string> row;while(std::getline(in,s,','))row.push_back(s);rows.push_back(row);}
    return rows;
}
static std::ofstream sweepEvidence,accuracyEvidence,harmonicEvidence,aliasEvidence;
static double val(const std::vector<std::string>& r,std::size_t n){return std::stod(r[n]);}
static void devices() {
    double error=0,biasError=0;
    for(const auto& r:csv("device.csv")) {
        Ts808::Unknowns v{};v[0]=val(r,0);v[1]=val(r,1);const auto d=device::currents(v,false);CHECK(d.valid);
        const double base=d.i[0]/device::bf+d.i[1]/device::br,collector=d.i[0]-(1+1/device::br)*d.i[1];
        for(auto p:{std::pair<double,double>{base,val(r,2)},{collector,val(r,3)}}) {
            const double e=std::abs(p.first-p.second);error=std::max(error,e);CHECK(e<1e-10+2e-5*std::abs(p.second));
        }
    }
    for(const auto& r:csv("diode.csv"))for(bool fault:{false,true}) {
        Ts808::Unknowns v{};v[4]=val(r,0);const auto d=device::currents(v,fault);CHECK(d.valid);
        if(fault)CHECK(d.diodes[1]==0);else CHECK(std::abs(d.diodes[1]-val(r,1))<1e-10+2e-5*std::abs(val(r,1)));
        CHECK(d.i[4]==d.diodes[1]-d.diodes[0]);
    }
    for(bool fault:{false,true})for(std::size_t p:{std::size_t(0),std::size_t(1),std::size_t(4)})for(double x:{-.6,-.1,0.,.1,.6}) {
        Ts808::Unknowns v{};v[p]=x;auto hi=v,lo=v;hi[p]+=1e-7;lo[p]-=1e-7;
        const auto d=device::currents(v,fault);const double numeric=(device::currents(hi,fault).i[p]-device::currents(lo,fault).i[p])/2e-7;
        CHECK(std::abs(numeric-d.g[p])<1e-13+2e-6*std::abs(d.g[p]));
    }
    // The closed-form amplifier response must solve the original current/pole
    // law, including slew/rail regions; a helper's own implementation is not truth.
    for(bool dc:{false,true})for(double rate:{96000.,192000.,384000.})for(double previous:{-3.001,-1.,0.,1.,3.001})for(double errorV:{-.1,-.001,-1e-6,0.,1e-6,.001,.1}) {
        const double step=1/rate;const auto r=device::response(errorV,previous,step,dc);const auto a=device::amplifier(errorV,r.value);
        const double lhs=dc?0:(r.value-previous)/step;CHECK(std::abs(lhs-a.rate)<.01);
    }
    for(const auto& r:csv("bias.csv")) {
        const float c=float(val(r,1));Ts808 m;CHECK(m.prepare(48000,{c,c,c,val(r,0)!=0}));
        for(std::size_t n=0;n<Ts808::kNodes;++n)biasError=std::max(biasError,std::abs(m.biasNodes()[n]-val(r,2+n)));
        CHECK(m.biasNodes()[Ts808::B1]>4&&m.biasNodes()[Ts808::E1]>3.3);
    }
    CHECK(biasError<1e-5);std::cout<<"Device peak_A="<<error<<" bias_peak_V="<<biasError<<'\n';
}
static void transients() {
    std::vector<double> errors;
    for(unsigned rate:{192000u,384000u,768000u,1536000u,705600u}) {
        const auto rows=csv(rate==705600?"transient-705600.csv":"transient.csv");
        Ts808 m;std::string key;unsigned next=0;double sum=0,peak=0,jackSum=0,jackPeak=0,currentPeak=0;std::size_t count=0,jackCount=0;
        const auto harmonicRows=csv("harmonics.csv");
        std::array<std::array<std::complex<double>,5>,2> sums{};
        std::array<double,2> minima{99,99},maxima{-99,-99};unsigned harmonicCount=0;
        auto finishHarmonics=[&](){
            if(rate!=768000||key.empty())return;
            for(const auto& row:harmonicRows)if(row[0]+row[1]==key){
                const unsigned node=row[5]=="clip"?0:1;
                CHECK(harmonicCount>0);
                for(unsigned h=0;h<5;++h){const double actual=2*std::abs(sums[node][h])/harmonicCount,expected=val(row,9+h);
                    CHECK(std::abs(actual-expected)<1e-4+.05*std::abs(expected));
                    if(harmonicEvidence)harmonicEvidence<<row[0]<<','<<row[1]<<','<<row[5]<<','<<(h==0?1:h==1?2:h==2?3:h==3?5:7)<<','<<actual<<','<<expected<<'\n';
                }
                const double peakBudget=node==0?.1:.001;CHECK(std::abs(minima[node]-val(row,6))<peakBudget&&std::abs(maxima[node]-val(row,7))<peakBudget);
            }
        };
        for(const auto& r:rows) {
            CHECK(r.size()==33);const auto group=r[0]+r[1];
            if(group!=key){finishHarmonics();sums={};minima={99,99};maxima={-99,-99};harmonicCount=0;key=group;next=0;CHECK(m.prepare(float(rate),{float(val(r,2)),.5f,.5f,val(r,1)!=0},Ts808Rate::Reference1x));}
            const unsigned target=unsigned(std::llround(val(r,6)*rate));
            for(;next<=target;++next) {
                guard::begin();const auto y=m.process(ts808lab::periodic(double(next)/rate,val(r,3),val(r,4),val(r,5)!=0));guard::end();
                CHECK(guard::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);
                const double time=double(next)/rate;
                if(rate==768000&&time>=.08&&time<.12){
                    const double values[]={m.nodes()[Ts808::Clip],m.nodes()[Ts808::Jack]};
                    const auto root=std::polar(1.,-2*ts808lab::pi*val(r,4)*time);std::complex<double> phase=1;unsigned slot=0;
                    for(unsigned h=1;h<=7;++h){phase*=root;if(h==4||h==6)continue;for(unsigned node=0;node<2;++node)sums[node][slot]+=values[node]*phase;++slot;}
                    for(unsigned node=0;node<2;++node){minima[node]=std::min(minima[node],values[node]);maxima[node]=std::max(maxima[node],values[node]);}++harmonicCount;
                }
            }
            for(std::size_t n=0;n<23;++n) {
                const double actual=n<12?m.nodes()[n]:n<14?m.internalVolts(n-12):m.capacitorVolts(n-14);
                const double e=std::abs(actual-val(r,7+n));sum+=e*e;peak=std::max(peak,e);++count;
                if(n==11){jackSum+=e*e;jackPeak=std::max(jackPeak,e);++jackCount;}
            }
            for(std::size_t n=0;n<2;++n)currentPeak=std::max(currentPeak,std::abs(m.diodeAmps(n)-val(r,30+n)));
            if(val(r,1))CHECK(m.diodeAmps(1)==0);
            if(m.failures()||m.outputClamps())std::cerr<<"Transient failure "<<key<<" rate "<<rate<<" t "<<val(r,6)<<" failures "<<m.failures()<<" clamps "<<m.outputClamps()<<" residual "<<m.residualVolts()<<" evals "<<m.maximumEvaluations()<<"\n";
            CHECK(m.failures()==0&&m.outputClamps()==0&&m.maximumEvaluations()<=1+Ts808::kUpdates*Ts808::kSearch);
        }
        finishHarmonics();
        errors.push_back(std::sqrt(sum/count));
        if(accuracyEvidence)accuracyEvidence<<rate<<','<<errors.back()<<','<<peak<<','<<std::sqrt(jackSum/jackCount)<<','<<jackPeak<<','<<currentPeak<<'\n';
        std::cout<<"Transient "<<rate<<" all_RMS/peak_V="<<errors.back()<<'/'<<peak<<" jack_RMS/peak_V="<<std::sqrt(jackSum/jackCount)<<'/'<<jackPeak<<" diode_peak_A="<<currentPeak<<'\n';
        if(rate>=705600)CHECK(errors.back()<.01&&peak<.1&&jackPeak<.001&&currentPeak<50e-6);
        else CHECK(errors.back()<.03&&peak<.5); // Explicit coarse-grid diagnostics, NOT final16x fidelity.
    }
    CHECK(errors[2]<.5*errors[0]&&errors[3]<errors[2]);
}
static void controls() {
    double worst=0;std::array<double,3> response{};double lowDrive=0,highDrive=0,lowTone=0,highTone=0;
    for(const auto& r:csv("ac.csv")) {
        Ts808 m;CHECK(m.prepare(192000,{float(val(r,1)),float(val(r,2)),float(val(r,3)),false},Ts808Rate::Reference1x));
        constexpr unsigned rate=192000;constexpr double amplitude=1e-5;
        const double f=val(r,4);std::array<std::complex<double>,4> bins{};unsigned count=0;
        for(unsigned n=0;n<rate*29/100;++n) {
            const double phase=2*ts808lab::pi*f*n/rate;m.process(float(amplitude*std::sin(phase)));
            if(n>=rate/4) {
                const std::complex<double> projection(std::sin(phase),std::cos(phase));
                const std::size_t nodes[]={Ts808::E1,Ts808::Clip,Ts808::ToneOut,Ts808::Jack};
                for(unsigned k=0;k<4;++k)bins[k]+=(m.nodes()[nodes[k]]-m.biasNodes()[nodes[k]])*projection;
                ++count;
            }
        }
        for(auto& v:bins)v*=2/(count*amplitude);
        if(sweepEvidence){sweepEvidence<<r[0]<<','<<r[1]<<','<<r[2]<<','<<r[3]<<','<<f;for(const auto& v:bins)sweepEvidence<<','<<v.real()<<','<<v.imag();sweepEvidence<<'\n';}
        for(unsigned k=0;k<4;++k) {
            const std::complex<double> expected(val(r,5+2*k),val(r,6+2*k));
            if(std::abs(expected)>1e-5){const double e=std::abs(bins[k]-expected)/std::abs(expected);worst=std::max(worst,e);CHECK(e<.05);}
            else CHECK(std::abs(bins[k])<1e-4);
        }
        if(r[0]=="nominal"&&(f==100||f==1000||f==10000))response[f==100?0:f==1000?1:2]=std::abs(bins[3]);
        if(f==1000&&r[0]=="drive0")lowDrive=std::abs(bins[1]);
        if(f==1000&&r[0]=="drive1")highDrive=std::abs(bins[1]);
        if(f==8000&&r[0]=="tone0")lowTone=std::abs(bins[3]);
        if(f==8000&&r[0]=="tone1")highTone=std::abs(bins[3]);
        CHECK(m.failures()==0&&m.outputClamps()==0);
    }
    CHECK(response[1]>1.5*response[0]&&response[1]>2*response[2]);CHECK(highDrive>4*lowDrive);CHECK(highTone>2*lowTone);
    std::cout<<"AC max_complex_relative="<<worst<<" jackGain100/1k/10k="<<response[0]<<'/'<<response[1]<<'/'<<response[2]<<'\n';
}
// Owned FFT reused, no additional analysis/runtime dependency.
static void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*ts808lab::pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
static void alias() {
    constexpr unsigned rate=96000,size=4096,bin=340;
    for(bool fault:{false,true}) {
        std::array<double,2> measured{};
        for(unsigned mode=0;mode<2;++mode) {
            Ts808 m;CHECK(m.prepare(rate,{.7f,1,.5f,fault},mode==0?Ts808Rate::Reference1x:Ts808Rate::Oversampled16x));
            std::vector<std::complex<double>> bins(size);double first=0,last=0;
            for(unsigned n=0;n<2*rate+size;++n) {
                const auto y=m.process(float(.15*std::sin(2*ts808lab::pi*bin*n/size)));
                if(n>=2*rate){bins[n-2*rate]=y;if(n<2*rate+size/2)first+=y;else last+=y;}
            }
            CHECK(m.failures()==0&&m.outputClamps()==0);CHECK(std::abs(first-last)/(size/2)<1e-6);
            fft(bins);double energy=0;
            for(unsigned k=1;k<size/2;++k) {
                bool harmonic=false;for(unsigned h=1;h*bin<size/2;++h)if(k==h*bin)harmonic=true;
                if(!harmonic)energy+=std::norm(bins[k]);
            }
            measured[mode]=10*std::log10(std::max(energy/std::norm(bins[bin]),1e-30));
            const double even=20*std::log10(std::max(std::abs(bins[2*bin]/bins[bin]),1e-15));
            if(aliasEvidence)aliasEvidence<<fault<<','<<(mode==0?"diagnostic1x":"normal16x")<<','<<rate<<','<<double(bin)*rate/size<<",0.15,"<<measured[mode]<<','<<even<<",0\n";
        }
        std::cout<<"Alias fault="<<fault<<" 1x/16x_dBc="<<measured[0]<<'/'<<measured[1]<<" improvement_dB="<<measured[0]-measured[1]<<'\n';
        CHECK(measured[1]<measured[0]-6);
    }
}
static void safety() {
    guard::selfTest();Ts808 a,b;CHECK(a.process(1)==0);
    for(unsigned rate:{44100u,48000u,96000u})for(bool fault:{false,true}) {
        CHECK(a.prepare(float(rate),{.5f,.5f,.5f,fault}));
        for(unsigned n=0;n<rate*2;++n) {
            const auto x=n<rate?ts808lab::attacks(double(n)/rate):ts808lab::note(double(n-rate)/rate);
            guard::begin();const auto y=a.process(x);guard::end();CHECK(guard::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);
        }
        double tailPeak=0;
        for(unsigned n=0;n<rate;++n){const auto y=a.process(0);CHECK(std::isfinite(y)&&std::abs(y)<=1);if(n>=rate*9/10)tailPeak=std::max(tailPeak,std::abs(double(y)));}
        CHECK(tailPeak<1e-4);CHECK(a.failures()==0&&a.outputClamps()==0);
        CHECK(a.prepare(float(rate),{}));CHECK(b.prepare(float(rate),{}));
        for(unsigned n=0;n<100;++n)CHECK(a.process(.03f)==b.process(.03f));
        for(float bad:{NAN,INFINITY,0.f,31999.f,44099.f,96001.f})CHECK(!a.prepare(bad));
        for(float bad:{NAN,INFINITY,-.01f,1.01f}) {
            CHECK(!a.prepare(float(rate),{bad,.5f,.5f}));CHECK(!a.prepare(float(rate),{.5f,bad,.5f}));CHECK(!a.prepare(float(rate),{.5f,.5f,bad}));
        }
        CHECK(!a.prepare(float(rate),{},static_cast<Ts808Rate>(99)));
        for(unsigned n=0;n<200;++n)CHECK(a.process(.03f)==b.process(.03f));
        const auto before=a.nodes();CHECK(a.process(NAN)==0&&a.nodes()==before);
        for(unsigned n=0;n<100;++n)CHECK(a.process(.03f)==b.process(.03f));
        a.stop();CHECK(a.process(1)==0);a.reset();b.reset();for(unsigned n=0;n<100;++n)CHECK(a.process(.03f)==b.process(.03f));
        CHECK(a.prepare(float(rate),{1,1,0,fault}));for(unsigned n=0;n<1000;++n)CHECK(a.process(.2f)==0);
        CHECK(a.prepare(float(rate),{1,1,1,fault}));
        const float edges[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),std::numeric_limits<float>::denorm_min(),0};
        for(unsigned n=0;n<2048;++n){guard::begin();const auto y=a.process(edges[n%7]);guard::end();CHECK(guard::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);}
        CHECK(a.inputClamps()>0&&a.invalidInputs()>0);
        a.reset();double zeroPeak=0;for(unsigned n=0;n<10000;++n)zeroPeak=std::max(zeroPeak,std::abs(double(a.process(0))));
        if(zeroPeak>=1e-8){std::cerr<<"Zero peak "<<zeroPeak<<" rate "<<rate<<" fault "<<fault<<'\n';}
        CHECK(zeroPeak<1e-8);
    }
    std::cout<<"Rates/guards/controls/rejection/reset/extremes/silence passed; state="<<sizeof(Ts808)<<'\n';
}
int main(int argc,char** argv){
    CHECK(argc==1||(argc==3&&std::string(argv[1])=="--evidence"));
    if(argc==3){const std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
        sweepEvidence.open(out/"stage-sweeps.csv");accuracyEvidence.open(out/"science-accuracy.csv");harmonicEvidence.open(out/"harmonic-comparison.csv");aliasEvidence.open(out/"alias.csv");
        for(auto* f:{&sweepEvidence,&accuracyEvidence,&harmonicEvidence,&aliasEvidence}){CHECK(bool(*f));*f<<std::setprecision(15);}
        sweepEvidence<<"case,drive,tone,level,frequency_hz,input_re,input_im,clip_re,clip_im,tone_re,tone_im,jack_re,jack_im\n";
        accuracyEvidence<<"integration_rate_hz,all_rms_V,all_peak_V,jack_rms_V,jack_peak_V,diode_peak_A\n";
        harmonicEvidence<<"case,fault,node,harmonic,actual_peak_V,spice_peak_V\n";
        aliasEvidence<<"fault,mode,rate_hz,frequency_hz,amplitude_V,alias_dBc,even_dBc,solver_failures\n";
    }
    devices();transients();controls();alias();safety();std::cout<<"TS808 checks="<<testChecks<<" passed\n";
}
