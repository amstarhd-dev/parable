#include "parable_klon.h"
#include "klon_device.h"
#include "klon_stimulus.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Klon;using parable::dsp::KlonControls;using parable::dsp::KlonRate;
namespace device=parable::dsp::klon_detail;namespace guard=activation_guard;
static std::vector<std::vector<std::string>> csv(const std::string& name) {
    std::ifstream f(std::string(KLON_FIXTURES)+"/"+name);CHECK(bool(f));std::string line;std::getline(f,line);std::vector<std::vector<std::string>> rows;
    while(std::getline(f,line)){std::stringstream in(line);std::string s;std::vector<std::string> row;while(std::getline(in,s,','))row.push_back(s);rows.push_back(row);}return rows;
}
static double val(const std::vector<std::string>& r,std::size_t k){return std::stod(r[k]);}
static std::ofstream accuracyEvidence,harmonicEvidence,sweepEvidence,aliasEvidence;
static void devices() {
    double currentError=0,biasError=0;
    for(const auto& row:csv("diode.csv")){
        const auto d=device::diode(val(row,0));CHECK(d.valid);
        const double error=std::abs(d.d3-val(row,1));currentError=std::max(currentError,error);CHECK(error<1e-8+2e-5*std::abs(val(row,1)));
        CHECK(std::abs(d.i+device::g0*val(row,0)-(d.d3-d.d2))<1e-12);
    }
    for(double v:{-.5,-.1,0.,.1,.5}){const auto d=device::diode(v);const double numeric=(device::diode(v+1e-7).i-device::diode(v-1e-7).i)/2e-7;CHECK(std::abs(d.g-numeric)<1e-11+2e-6*std::abs(d.g));}
    CHECK(!device::diode(std::numeric_limits<double>::quiet_NaN()).valid&&!device::diode(20).valid);
    // Validate exact self-feedback elimination against the original continuous
    // pole/current-limited charging/rail law, independently evaluated here.
    for(bool dc:{false,true})for(std::size_t amp:{0u,2u})for(double self:{-.999,-.1,-.001})for(double prev:{-11.6,-3.,0.,3.,10.2})for(double error:{-2.,-.01,-1e-6,0.,1e-6,.01,2.}) {
        const double step=2/(3*192000.);const auto r=device::response(error,self,prev,step,dc,amp);const double lo=amp<2?-3:-11.6,hi=amp<2?3:10.2;
        const double rate=std::clamp(device::wp*(device::gain*(error+self*r.value)-r.value),-device::slew,device::slew)-device::railG*(r.value-std::clamp(r.value,lo,hi));
        CHECK(std::abs((dc?0:(r.value-prev)/step)-rate)<.1);
    }
    for(const auto& row:csv("bias.csv"))if(val(row,1)==0||val(row,1)==100000) {
        Klon m;CHECK(m.prepare(48000,{float(val(row,0)),.5f,.5f,val(row,1)!=0}));
        for(std::size_t k=0;k<Klon::kNodes;++k)biasError=std::max(biasError,std::abs(m.biasNodes()[k]-val(row,2+k)));
        for(std::size_t k=0;k<Klon::kCaps;++k)biasError=std::max(biasError,std::abs(m.capacitorVolts(k)-val(row,27+k)));
    }
    CHECK(biasError<1e-5);std::cout<<"Device current peak_A="<<currentError<<" bias peak_V="<<biasError<<'\n';
}
static void transients() {
    std::vector<double> errors;
    for(unsigned rate:{529200u,576000u,768000u,1152000u}) {
        const auto rows=csv("transient-"+std::to_string(rate)+".csv");const auto harmonicRows=csv("harmonics.csv");
        Klon m;std::string key;unsigned next=0;double sum=0,peak=0,jackSum=0,jackPeak=0,ordinaryJackSum=0,mixPeak=0,hardPeak=0,hardJackPeak=0;
        std::size_t count=0,jackCount=0,ordinaryJackCount=0;std::array<std::array<std::complex<double>,5>,6> sums{};std::array<double,6> minima{},maxima{};unsigned harmonicCount=0;
        const auto finish=[&](){
            if(key.empty()||rate!=768000)return;
            for(const auto& row:harmonicRows)if(row[0]+row[1]==key) {
                const unsigned node=row[2]=="clip"?0:row[2]=="so"?1:row[2]=="out"?2:row[2]=="drive_A"?3:row[2]=="clean1_A"?4:5;
                CHECK(harmonicCount>0);
                for(unsigned h=0;h<5;++h){const double actual=2*std::abs(sums[node][h])/harmonicCount,expected=val(row,7+h),floor=node<3?1e-4:1e-8;
                    CHECK(std::abs(actual-expected)<floor+.05*std::abs(expected));
                    if(harmonicEvidence)harmonicEvidence<<row[0]<<','<<row[1]<<','<<row[2]<<','<<(h==0?1:h==1?2:h==2?3:h==3?5:7)<<','<<actual<<','<<expected<<'\n';}
                CHECK(std::abs(minima[node]-val(row,4))<(node<3?.05:1e-5));CHECK(std::abs(maxima[node]-val(row,5))<(node<3?.05:1e-5));
            }
        };
        for(const auto& row:rows) {
            CHECK(row.size()==38);const auto group=row[0]+row[1];
            if(group!=key){finish();key=group;next=0;sums={};minima.fill(99);maxima.fill(-99);harmonicCount=0;CHECK(m.prepare(float(rate),{float(val(row,2)),float(val(row,3)),float(val(row,4)),val(row,1)!=0},KlonRate::Reference1x));}
            // Parse fixed fixture parameters once, preserving all native samples
            // and the original electrical/callback checks at their exact points.
            const double amplitude=val(row,5),frequency=val(row,6);const bool triangle=val(row,7)!=0;
            const auto target=unsigned(std::llround(val(row,8)*rate));
            for(;next<=target;++next){const double t=double(next)/rate;guard::begin();const auto y=m.process(klonlab::periodic(t,amplitude,frequency,triangle));guard::end();CHECK(guard::clean()&&std::isfinite(y)&&std::abs(y)<=1);
                CHECK(std::abs(m.mixResidualAmps())<1e-10);const auto& p=m.feedbackContributions();CHECK(std::abs(p[0]+p[1]+p[2]-m.nodes()[Klon::SumOut]+m.nodes()[Klon::SumMinus])<1e-5);
                if(rate==768000&&next>=30720&&next<46080){const auto& c=m.mixCurrents();const double values[]={m.nodes()[Klon::Clip],m.nodes()[Klon::SumOut],m.nodes()[Klon::Jack],c[0],c[1],c[2]};const auto root=std::polar(1.,-2*klonlab::pi*frequency*t);std::complex<double> phase=1;unsigned slot=0;
                    for(unsigned h=1;h<=7;++h){phase*=root;if(h==4||h==6)continue;for(unsigned n=0;n<6;++n)sums[n][slot]+=values[n]*phase;++slot;}
                    for(unsigned n=0;n<6;++n){minima[n]=std::min(minima[n],values[n]);maxima[n]=std::max(maxima[n],values[n]);}++harmonicCount;}
            }
            for(std::size_t k=0;k<25;++k){const double actual=k<Klon::kNodes?m.nodes()[k]:m.amplifierVolts(k-Klon::kNodes);const double e=std::abs(actual-val(row,10+k));sum+=e*e;++count;
                if(row[0]=="slew")hardPeak=std::max(hardPeak,e);else peak=std::max(peak,e);
                if(k==Klon::Jack){jackSum+=e*e;++jackCount;if(row[0]=="slew")hardJackPeak=std::max(hardJackPeak,e);else {jackPeak=std::max(jackPeak,e);ordinaryJackSum+=e*e;++ordinaryJackCount;}}}
            for(std::size_t k=0;k<3;++k)mixPeak=std::max(mixPeak,std::abs(m.mixCurrents()[k]-val(row,35+k)));
            if(m.failures()||m.outputClamps())std::cerr<<"Containment "<<key<<" rate"<<rate<<" t"<<val(row,8)<<" residual"<<m.residualVolts()<<" failures"<<m.failures()<<'\n';
            CHECK(m.failures()==0&&m.outputClamps()==0&&m.maximumEvaluations()<=1+Klon::kUpdates*Klon::kSearch);
        }
        finish();const double ordinaryJackRms=std::sqrt(ordinaryJackSum/ordinaryJackCount);const double rms=std::sqrt(sum/count);errors.push_back(rms);
        std::cout<<"Transient"<<rate<<" stage RMS/ordinary_peak/harsh_peak_V="<<rms<<'/'<<peak<<'/'<<hardPeak<<" jack RMS/ordinary_peak/harsh_peak_V="<<std::sqrt(jackSum/jackCount)<<'/'<<jackPeak<<'/'<<hardJackPeak<<" ordinary_jack_RMS_V="<<ordinaryJackRms<<" mix_peak_A="<<mixPeak<<'\n';
        if(accuracyEvidence)accuracyEvidence<<rate<<','<<rms<<','<<peak<<','<<hardPeak<<','<<std::sqrt(jackSum/jackCount)<<','<<jackPeak<<','<<hardJackPeak<<','<<mixPeak<<','<<ordinaryJackRms<<'\n';
        CHECK(ordinaryJackRms<.001&&rms<.01&&peak<.05&&hardPeak<.2&&jackPeak<.005&&hardJackPeak<.01);
    }
    CHECK(errors[2]<errors[1]&&errors[3]<errors[2]);
}
static void controls() {
    constexpr double amplitude=1e-5;constexpr unsigned rate=576000;double worst=0;unsigned comparisons=0;
    std::array<double,2> lowGain{},highGain{},cleanLow{},cleanHigh{};
    for(const auto& row:csv("ac.csv")) {
        const double hz=val(row,4);if(std::abs(hz-100)>1&&std::abs(hz-1000)>1&&std::abs(hz-10000)>1)continue;
        Klon m;CHECK(m.prepare(rate,{float(val(row,0)),float(val(row,1)),float(val(row,2)),val(row,3)!=0},KlonRate::Reference1x));
        const auto settle=unsigned(rate*.25),length=unsigned(std::llround(rate*std::max(.02,4/hz)));std::array<std::complex<double>,7> bins{};
        for(unsigned n=0;n<settle+length;++n){const double t=double(n)/rate;m.process(float(amplitude*std::sin(2*klonlab::pi*hz*t)));
            if(n>=settle){const auto phase=std::polar(1.,-2*klonlab::pi*hz*t);const auto& v=m.nodes();const auto& c=m.mixCurrents();const double values[]={v[Klon::Buffer]-m.biasNodes()[Klon::Buffer],v[Klon::DriveOut]-m.biasNodes()[Klon::DriveOut],v[Klon::SumOut]-m.biasNodes()[Klon::SumOut],v[Klon::Jack],c[0],c[1],c[2]};for(unsigned k=0;k<7;++k)bins[k]+=values[k]*phase;}}
        const std::size_t columns[]={1,7,13,19,25,26,27};
        for(unsigned k=0;k<7;++k){const std::complex<double> actual=bins[k]*std::complex<double>(0,2./(amplitude*length)),expected={val(row,5+2*columns[k]),val(row,6+2*columns[k])};
            const double relative=std::abs(actual-expected)/std::max(1e-10,std::abs(expected));worst=std::max(worst,relative);if(!(std::abs(actual-expected)<(k<4?1e-4:1e-9)+.05*std::abs(expected)))std::cerr<<"AC mismatch gain/treble/level/fault/freq/signal="<<row[0]<<'/'<<row[1]<<'/'<<row[2]<<'/'<<row[3]<<'/'<<hz<<'/'<<k<<" actual "<<actual<<" spice "<<expected<<" rel "<<relative<<'\n';CHECK(std::abs(actual-expected)<(k<4?1e-4:1e-9)+.05*std::abs(expected));++comparisons;
            if(sweepEvidence)sweepEvidence<<row[0]<<','<<row[1]<<','<<row[2]<<','<<row[3]<<','<<hz<<','<<k<<','<<actual.real()<<','<<actual.imag()<<','<<expected.real()<<','<<expected.imag()<<'\n';
            if(hz>999&&hz<1001&&val(row,1)==.5&&val(row,2)==.5){const auto f=unsigned(val(row,3));if(val(row,0)==0){if(k==3)lowGain[f]=std::abs(actual);if(k==6)cleanLow[f]=std::abs(actual);}if(val(row,0)==1){if(k==3)highGain[f]=std::abs(actual);if(k==6)cleanHigh[f]=std::abs(actual);}}
        }
        CHECK(m.failures()==0&&m.outputClamps()==0);
    }
    CHECK(comparisons==378);CHECK(lowGain[1]<.5*lowGain[0]&&highGain[1]>1.1*highGain[0]);CHECK(cleanLow[1]<.2*cleanLow[0]);
    std::cout<<"Complex AC comparisons="<<comparisons<<" maximum_relative="<<worst<<" low-gain healthy/fault="<<lowGain[0]<<'/'<<lowGain[1]<<" high-gain="<<highGain[0]<<'/'<<highGain[1]<<'\n';
}
static void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*klonlab::pi/length);for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
static void aliasing() {
    constexpr unsigned rate=96000,size=4096,bin=340;const double hz=double(rate)*bin/size;
    for(bool fault:{false,true}){double db[2]{};for(unsigned mode=0;mode<2;++mode){Klon m;CHECK(m.prepare(rate,{.7f,1,.5f,fault},mode==0?KlonRate::Reference1x:KlonRate::Oversampled12x));for(unsigned n=0;n<2*rate;++n)m.process(klonlab::periodic(double(n)/rate,.15,hz));std::vector<std::complex<double>> bins(size);for(unsigned n=0;n<size;++n)bins[n]=m.process(klonlab::periodic(double(n+2*rate)/rate,.15,hz));fft(bins);double peak=0;
        for(unsigned k=1;k<size/2;++k){if(k%bin!=0)peak=std::max(peak,std::abs(bins[k]));}
        db[mode]=20*std::log10(std::max(peak,1e-30)/std::abs(bins[bin]));CHECK(m.failures()==0&&m.outputClamps()==0);}
        std::cout<<"Alias fault"<<fault<<" 1x/12x_dBc="<<db[0]<<'/'<<db[1]<<" improvement="<<db[0]-db[1]<<'\n';if(aliasEvidence)aliasEvidence<<fault<<','<<hz<<','<<db[0]<<','<<db[1]<<','<<db[0]-db[1]<<'\n';CHECK(db[1]<db[0]-15);}
}
static void safety() {
    Klon stopped;CHECK(stopped.process(1)==0);CHECK(!stopped.prepare(0));CHECK(!stopped.prepare(48000,{2,.5f,.5f,false}));
    for(float rate:{44100.f,48000.f,96000.f})for(bool fault:{false,true}) {
        for(float g:{0.f,.5f,1.f})for(float t:{0.f,.5f,1.f})for(float l:{0.f,.5f,1.f}){Klon m;CHECK(m.prepare(rate,{g,t,l,fault}));for(unsigned n=0;n<2048;++n){guard::begin();const auto y=m.process(klonlab::attacks(double(n)/rate)+klonlab::periodic(double(n)/rate,.15,1000));guard::end();CHECK(guard::clean()&&std::isfinite(y)&&std::abs(y)<=1);if(l==0)CHECK(y==0);}CHECK(m.failures()==0&&m.outputClamps()==0);}
        Klon m;CHECK(m.prepare(rate,{.5f,.5f,.5f,fault}));for(unsigned n=0;n<unsigned(rate*3);++n){const auto y=m.process(n<unsigned(rate*2)?klonlab::note(double(n)/rate):0);CHECK(std::isfinite(y)&&std::abs(y)<=1);if(n+1==unsigned(rate*3))CHECK(std::abs(y)<1e-4);}CHECK(m.failures()==0&&m.outputClamps()==0);
        m.reset();std::vector<float> before(2048);for(unsigned n=0;n<before.size();++n)before[n]=m.process(klonlab::attacks(double(n)/rate));m.reset();for(unsigned n=0;n<before.size();++n)CHECK(before[n]==m.process(klonlab::attacks(double(n)/rate)));
        auto copy=m;CHECK(!m.prepare(std::numeric_limits<float>::quiet_NaN()));for(unsigned n=0;n<100;++n)CHECK(m.process(.01f)==copy.process(.01f));
        for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){copy=m;CHECK(m.process(bad)==0);for(unsigned n=0;n<100;++n)CHECK(m.process(.02f)==copy.process(.02f));}
        for(float v:{std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),std::numeric_limits<float>::denorm_min(),2.f,-2.f}){guard::begin();const float y=m.process(v);guard::end();CHECK(guard::clean()&&std::isfinite(y)&&std::abs(y)<=1);CHECK(m.maximumEvaluations()<=1+Klon::kUpdates*Klon::kSearch);}
        m.stop();CHECK(m.process(1)==0);m.reset();for(unsigned n=0;n<10000;++n)CHECK(std::abs(m.process(0))<1e-8);CHECK(m.failures()==0);
    }
    std::cout<<"State="<<sizeof(Klon)<<" bytes; rates/endpoints/dynamics/silence/reset/invalid input and callback guards passed\n";
}
int main(int argc,char** argv) {
    if(argc==3&&std::string(argv[1])=="--evidence") {std::filesystem::create_directories(argv[2]);const auto root=std::filesystem::path(argv[2]);accuracyEvidence.open(root/"science-accuracy.csv");harmonicEvidence.open(root/"harmonic-comparison.csv");sweepEvidence.open(root/"stage-sweeps.csv");aliasEvidence.open(root/"alias.csv");for(auto* p:{&accuracyEvidence,&harmonicEvidence,&sweepEvidence,&aliasEvidence}){CHECK(bool(*p));*p<<std::setprecision(15);}accuracyEvidence<<"integration_hz,stage_rms_V,ordinary_peak_V,harsh_peak_V,jack_rms_V,ordinary_jack_peak_V,harsh_jack_peak_V,mix_peak_A,ordinary_jack_rms_V\n";harmonicEvidence<<"case,fault,node,harmonic,actual,spice\n";sweepEvidence<<"gain,treble,level,fault,hz,signal,native_real,native_imag,spice_real,spice_imag\n";aliasEvidence<<"fault,hz,1x_dBc,12x_dBc,improvement_dB\n";}
    else CHECK(argc==1);
    guard::selfTest();devices();transients();controls();aliasing();safety();CHECK(guard::clean());std::cout<<"All "<<testChecks<<" Klon checks passed\n";
}
