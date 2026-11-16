#include "parable_coupled_rooms.h"
#include "activation_guard.h"
#include "test_check.h"
// Reuse the existing FFT; other inline RAT measurement helpers are not called.
#include "rat_measure.h"
#include <array>
#include <limits>
using parable::soundforge::CoupledRooms;
using parable::soundforge::RoomControls;
using parable::soundforge::RoomOutput;
constexpr double pi=3.14159265358979323846;
struct Owned {
    std::vector<float> storage;
    CoupledRooms model;
    explicit Owned(float rate,RoomControls c={}):storage(CoupledRooms::requiredSamples(rate)) {
        CHECK(model.prepare(rate,storage.data(),storage.size(),c));
    }
};
static void reference() {
    std::ifstream f(std::string(ROOM_FIXTURES)+"/impulse.csv");CHECK(bool(f));
    std::string row;std::getline(f,row);int previous=-1;unsigned next=0;double peak=0;unsigned points=0;
    std::vector<float> buffer(CoupledRooms::kMaxSamples);CoupledRooms m;RoomOutput y;
    while(std::getline(f,row)) {
        const auto cells=ratlab::fields(row);CHECK(cells.size()==6);
        const int fault=std::stoi(cells[0]),rate=std::stoi(cells[1]);const int key=rate+fault*100000;
        if(key!=previous){CHECK(m.prepare(float(rate),buffer.data(),buffer.size(),{.85f,1.f,fault!=0}));next=0;previous=key;}
        const unsigned target=unsigned(std::stoul(cells[2]));
        for(;next<=target;++next)y=m.process(next==0?.5f:0);
        const float actual[]={y.mono,y.a,y.b};
        for(unsigned i=0;i<3;++i){peak=std::max(peak,std::abs(actual[i]-std::stod(cells[i+3])));++points;}
    }
    CHECK(points>10000&&peak<2.e-6);
    std::cout<<"Independent dense reference points="<<points<<" peak_error="<<peak<<'\n';
}
static void rooms() {
    for(float rate:{44100.f,48000.f,96000.f}) {
        Owned closed(rate,{0,1,false}),coupled(rate,{.85f,1,false}),reverse(rate,{.85f,1,false});
        double aEnergy=0,bEnergy=0,reverseEnergy=0;unsigned onsetA=0,onsetB=0;
        for(unsigned n=0;n<unsigned(rate);++n) {
            const float x=n==0?.5f:0;const auto off=closed.model.process(x),on=coupled.model.process(x),back=reverse.model.process(0,x);
            CHECK(off.b==0);CHECK(back.a==0||n>=coupled.model.length(0)+coupled.model.length(4));
            aEnergy+=double(off.a)*off.a;bEnergy+=double(on.b)*on.b;reverseEnergy+=double(back.a)*back.a;
            if(!onsetA&&on.a!=0)onsetA=n;
            if(!onsetB&&on.b!=0)onsetB=n;
        }
        CHECK(aEnergy>0&&bEnergy>1.e-5&&reverseEnergy>1.e-5);
        CHECK(onsetA==coupled.model.length(0));
        CHECK(onsetB==coupled.model.length(0)+coupled.model.length(4));
        // Each uncoupled room has its own first path and absorption/decay.
        closed.model.reset();unsigned firstB=0;
        for(unsigned n=0;n<unsigned(rate*.1f);++n){const auto y=closed.model.process(0,n==0?.5f:0);CHECK(y.a==0);if(!firstB&&y.b!=0)firstB=n;}
        CHECK(firstB==closed.model.length(4)&&firstB>onsetA*2);
        std::cout<<"rate="<<rate<<" buffer_bytes="<<coupled.model.bufferBytes()<<" onsets_A/B="<<onsetA<<'/'<<onsetB<<" B_energy="<<bEnergy<<'\n';
    }
}
static void decaySafety() {
    activation_guard::selfTest();bool safe=true;
    for(float rate:{44100.f,48000.f,96000.f})for(float c:{0.f,.85f,1.f})for(bool fault:{false,true}) {
        Owned o(rate,{c,1,fault});double early=0,late=0;float peak=0;
        activation_guard::begin();
        for(unsigned n=0;n<unsigned(rate*6);++n) {
            const auto y=o.model.process(n==0?.5f:0);
            safe&=std::isfinite(y.mono)&&std::isfinite(y.a)&&std::isfinite(y.b);
            peak=std::max(peak,std::abs(y.mono));
            if(n>rate*.2&&n<rate*.7)early+=double(y.mono)*y.mono;
            if(n>rate*5.5)late+=double(y.mono)*y.mono;
        }
        activation_guard::end();CHECK(safe&&activation_guard::clean()&&peak<1&&late<early*1.e-5);
        o.model.reset();std::array<float,4096> first{};
        for(unsigned i=0;i<first.size();++i)first[i]=o.model.process(i==0?.1f:0).mono;
        o.model.reset();for(unsigned i=0;i<first.size();++i)CHECK(first[i]==o.model.process(i==0?.1f:0).mono);
        o.model.reset();for(unsigned n=0;n<2048;++n)CHECK(o.model.process(0).mono==0);
    }
    std::cout<<"18 rate/coupling/fault decay corners: final-window energy < early*1e-5; callback guards clean\n";
    Owned a(48000),b(48000);
    for(unsigned n=0;n<3000;++n){a.model.process(.1f);b.model.process(.1f);}
    CHECK(!a.model.prepare(32000,a.storage.data(),a.storage.size()));
    CHECK(!a.model.prepare(NAN,a.storage.data(),a.storage.size()));
    CHECK(!a.model.prepare(48000,nullptr,a.storage.size()));
    CHECK(!a.model.prepare(48000,a.storage.data(),1));
    CHECK(!a.model.setControls({NAN,.5f,false})&&!a.model.setControls({.5f,2,false}));
    CHECK(a.model.process(NAN).mono==0&&a.model.process(INFINITY).mono==0);
    for(unsigned n=0;n<1024;++n)CHECK(a.model.process(.1f).mono==b.model.process(.1f).mono);
    for(unsigned n=0;n<48000;++n)CHECK(std::isfinite(a.model.process(n%2?std::numeric_limits<float>::max():-std::numeric_limits<float>::max()).mono));
    CHECK(CoupledRooms::requiredSamples(96000)<=CoupledRooms::kMaxSamples);
    CoupledRooms empty;CHECK(empty.process(1).mono==0);
}
static void controlsAndSpectrum() {
    Owned a(48000),baseline(48000);float previous=0,baselinePrevious=0;double stepError=0;
    for(unsigned n=0;n<96000;++n) {
        if(n==48000)CHECK(a.model.setControls({0,1,true}));
        const float x=float(.1*std::sin(2*pi*220*n/48000));
        const float y=a.model.process(x).mono,z=baseline.model.process(x).mono;
        if(n==48000)stepError=std::abs((y-previous)-(z-baselinePrevious));
        CHECK(std::isfinite(y));previous=y;baselinePrevious=z;
    }
    CHECK(stepError<.0001);std::cout<<"Control-change first-frame excess step="<<stepError<<'\n';
    constexpr unsigned count=4096,bin=683;
    for(bool fault:{false,true}) {
        Owned o(48000,{.85f,1,fault});std::vector<std::complex<double>> bins(count);
        for(unsigned n=0;n<288000+count;++n){const auto y=o.model.process(float(.1*std::sin(2*pi*bin*n/count))).mono;if(n>=288000)bins[n-288000]=y;}
        ratlab::fft(bins);double unwanted=0;
        for(unsigned k=1;k<count/2;++k)if(k!=bin)unwanted+=std::norm(bins[k]);
        const double spur=10*std::log10(std::max(unwanted/std::norm(bins[bin]),1e-30));
        std::cout<<"Fixed-control linear 8003.9Hz unwanted/fundamental fault="<<fault<<" dB="<<spur<<'\n';CHECK(spur<-85);
    }
}
static void oneWire() {
    Owned healthy(48000,{.85f,1,false}),fault(48000,{.85f,1,true});
    Owned offHealthy(48000,{0,1,false}),offFault(48000,{0,1,true});
    double healthyEnergy=0,faultEnergy=0,cross=0;unsigned firstA=0,firstB=0;
    for(unsigned n=0;n<96000;++n) {
        const float x=n==0?.5f:0;
        const auto h=healthy.model.process(x),f=fault.model.process(x);
        CHECK(offHealthy.model.process(x).mono==offFault.model.process(x).mono);
        if(!firstA&&h.a!=f.a)firstA=n;
        if(!firstB&&h.b!=f.b)firstB=n;
        healthyEnergy+=double(h.mono)*h.mono;faultEnergy+=double(f.mono)*f.mono;cross+=double(h.mono)*f.mono;
    }
    CHECK(firstA==2*healthy.model.length(0)+healthy.model.length(4));
    CHECK(firstB==2*healthy.model.length(0)+2*healthy.model.length(4));
    // Residual after best constant gain fit: distinguish comb changes from volume.
    const double residual=std::sqrt(std::max(0.,1-cross*cross/(healthyEnergy*faultEnergy)));
    CHECK(residual>.25);
    std::cout<<"Single-return fault first differences A/B="<<firstA<<'/'<<firstB<<" gain-fitted impulse residual="<<residual<<'\n';
    healthy.model.reset();fault.model.reset();
    constexpr unsigned size=262144;
    std::vector<std::complex<double>> h(size),f(size);
    for(unsigned n=0;n<size;++n){h[n]=healthy.model.process(n==0?.5f:0).mono;f[n]=fault.model.process(n==0?.5f:0).mono;}
    ratlab::fft(h);ratlab::fft(f);unsigned counted=0,changed=0;std::vector<double> delta;
    for(unsigned k=1;k<size/2;++k) {
        const double hz=48000.*k/size;
        if(hz<80||hz>10000||std::abs(h[k])<=.01)continue;
        const double db=20*std::log10(std::max(std::abs(f[k]),1.e-12)/std::abs(h[k]));
        delta.push_back(db);++counted;if(std::abs(db)>3)++changed;
    }
    std::sort(delta.begin(),delta.end());const double fraction=double(changed)/counted;
    CHECK(counted>10000&&fraction>.1);
    std::cout<<"80Hz-10kHz modal response: fraction >3dB="<<fraction<<" delta5/50/95percentile_dB="<<delta[delta.size()/20]<<'/'<<delta[delta.size()/2]<<'/'<<delta[delta.size()*19/20]<<" (healthy magnitude>.01; FFT262144)\n";
}
int main(){reference();rooms();decaySafety();controlsAndSpectrum();oneWire();std::cout<<"Coupled room checks="<<testChecks<<" passed\n";}
