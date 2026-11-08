#include "parable_drive.h"
#include "lab_wav.h"
#include <array>
#include <complex>
#include <iomanip>
#include <iostream>
#include <numeric>

using parable::dsp::Drive;
using parable::dsp::DriveParameters;
using parable::dsp::DriveRate;
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr std::size_t nfft = 8192;
struct Stats { double rms = 0, peak = 0, mean = 0; };
Stats stats(const std::vector<float>& x) {
    Stats s;
    for (float v : x) { lab::require(std::isfinite(v),"Nonfinite DSP output"); s.rms += double(v)*v; s.peak=std::max(s.peak,std::fabs(double(v))); s.mean+=v; }
    s.rms=std::sqrt(s.rms/x.size()); s.mean/=x.size(); return s;
}
double db(double x) { return 20*std::log10(std::max(x,1e-15)); }
std::vector<float> render(const std::vector<float>& input, float rate, DriveParameters p, DriveRate mode = DriveRate::Oversampled4x) {
    Drive d; lab::require(d.configure(rate,p,mode),"Invalid lab DSP configuration");
    std::vector<float> out(input.size()); d.processBlock(input.data(),out.data(),out.size()); return out;
}
lab::Wave synthetic() {
    lab::Wave w; w.samples.resize(w.rate*9);
    const double strings[] = {82.4069,110,146.8324,195.9977,246.9417,329.6276};
    for (std::size_t i = 0; i < w.samples.size(); ++i) {
        const double t = double(i)/w.rate; double x = 0;
        for (int event = 0; event < 5; ++event) {
            const bool strum = event >= 2; const double start = .25+event*1.55;
            for (int string = 0; string < (strum ? 6 : 1); ++string) {
                const double age = t-start-string*.012;
                if (age < 0 || age > 2) continue;
                const double f = strings[string] * (event==1 ? 1.5 : 1.);
                const double release = age <= 1.9 ? 1. : .5*(1+std::cos(pi*(age-1.9)/.1));
                for (int harmonic=1; harmonic<=10; ++harmonic)
                    x += release * (event%2 ? .55 : 1.) * (1-std::exp(-age*600))
                         * std::exp(-age*(2.2+.26*harmonic)) / std::pow(harmonic,1.3)
                         * std::sin(2*pi*f*harmonic*age+.13*harmonic*harmonic);
            }
        }
        w.samples[i] = static_cast<float>(x);
    }
    const auto s = stats(w.samples); for(auto& x:w.samples)x=static_cast<float>(x*.6/s.peak);
    return w;
}
void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){std::size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){
        const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w; a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;
        }}
    }
}
std::vector<std::complex<double>> spectrum(unsigned bin, double amplitude, DriveParameters p, DriveRate mode) {
    Drive d; lab::require(d.configure(48000,p,mode),"Cannot configure measurement");
    std::vector<std::complex<double>> a(nfft);
    for(std::size_t i=0;i<3*nfft;++i){
        const float y=d.process(static_cast<float>(amplitude*std::sin(2*pi*bin*i/nfft)));
        if(i>=2*nfft)a[i-2*nfft]=y;
    }
    fft(a); return a;
}
double aliasDb(unsigned bin, const std::vector<std::complex<double>>& a) {
    double alias=0;
    for(std::size_t i=1;i<nfft/2;++i){
        bool harmonic=false;
        for(unsigned h=1;h*bin<nfft/2;++h) if(i==h*bin) harmonic=true;
        if(!harmonic)alias+=std::norm(a[i]);
    }
    return 10*std::log10(std::max(alias/std::norm(a[bin]),1e-30));
}
struct Alias { unsigned bin; double one, four; };
struct Voicing { double lowThd, highThd, symmetricSecond, biasedSecond, toneDifference; };
Voicing voicingMeasurements() {
    const auto harmonic=[](const std::vector<std::complex<double>>& a,unsigned h){return db(std::abs(a[171*h])/std::abs(a[171]));};
    const auto thd=[](const std::vector<std::complex<double>>& a){
        double other=0;for(unsigned i=1;i<nfft/2;++i)if(i!=171)other+=std::norm(a[i]);
        return 10*std::log10(std::max(other/std::norm(a[171]),1e-30));
    };
    const auto low=spectrum(171,.1,{.05f,1,0,.6f},DriveRate::Oversampled4x);
    const auto high=spectrum(171,.1,{.9f,1,0,.6f},DriveRate::Oversampled4x);
    const auto biased=spectrum(171,.1,{.9f,1,.8f,.6f},DriveRate::Oversampled4x);
    const auto dark=spectrum(1003,.005,{0,0,0,.6f},DriveRate::Oversampled4x);
    const auto bright=spectrum(1003,.005,{0,1,0,.6f},DriveRate::Oversampled4x);
    lab::require(std::abs(biased[0])/nfft<1e-5,"Asymmetric steady signal must reject output DC");
    Voicing v{thd(low),thd(high),harmonic(high,2),harmonic(biased,2),db(std::abs(bright[1003])/std::abs(dark[1003]))};
    lab::require(v.highThd>v.lowThd+10,"Drive must change harmonic content at matched fundamental");
    lab::require(v.biasedSecond>v.symmetricSecond+30,"Asymmetry must create even harmonic content");
    lab::require(v.toneDifference>15,"Tone must change upper-band response");
    return v;
}
std::vector<Alias> aliasMeasurements() {
    std::vector<Alias> rows;
    for(unsigned bin:{701u,1003u,1501u}){
        const DriveParameters p{.85f,1,.5f,.6f};
        const double one=aliasDb(bin,spectrum(bin,.4,p,DriveRate::Reference1x));
        const double four=aliasDb(bin,spectrum(bin,.4,p,DriveRate::Oversampled4x));
        lab::require(one-four>15,"4x alias reduction fell below 15 dB fixture gate");
        rows.push_back({bin,one,four});
    }
    return rows;
}
// Independent double reference evaluates the implicit equation at every sample;
// the implementation instead interpolates a precomputed float table.
double referenceError() {
    const DriveParameters p{.6f,.8f,.7f,.7f}; Drive d;
    lab::require(d.configure(48000,p,DriveRate::Reference1x),"Reference configuration");
    const auto coeff=[](double f){return 1-std::exp(-2*pi*f/48000);};
    double dcIn=0,pre=0,t1=0,t2=0,dcOut=0,error=0;
    for(int i=0;i<8192;++i){
        const float input=static_cast<float>(.5*std::sin(2*pi*173*i/48000));
        dcIn+=coeff(80)*(input-dcIn); pre+=coeff(6000)*(input-dcIn-pre);
        const double u=std::clamp(pre*(1.+39.*p.drive*p.drive),-16.,16.);
        double lo=-2,hi=2;
        for(int j=0;j<64;++j){const double v=(lo+hi)/2;
            const double current=v+.025*(std::exp(v/(.18*(1+.35*p.bias)))-std::exp(-v/(.18*(1-.35*p.bias))));
            if(current<u)lo=v;else hi=v;
        }
        const double v=(lo+hi)/2; const double tone=coeff(1200+6800*p.tone);
        t1+=tone*(v-t1);t2+=tone*(t1-t2);dcOut+=coeff(20)*(t2-dcOut);
        error=std::max(error,std::fabs(d.process(input)-(t2-dcOut)*p.level));
    }
    lab::require(error<.0003,"Table/float path differs from independent exact-solver reference");return error;
}
void selfTest() {
    const auto a=aliasMeasurements(); const double err=referenceError(); const auto voice=voicingMeasurements();
    // Known FFT bins/powers, independent of the drive, protect the alias metric.
    std::vector<std::complex<double>> pure(nfft);
    for(std::size_t i=0;i<nfft;++i)pure[i]=std::sin(2*pi*1003*i/nfft);
    fft(pure); lab::require(std::fabs(std::abs(pure[1003])-nfft/2)<1e-5,"FFT amplitude reference");
    lab::require(aliasDb(1003,pure)<-200,"FFT coherent pure sine floor");
    lab::Wave w{44100,{0,.25f,-.5f,.8f}}; const auto bytes=lab::encode(w); const auto decoded=lab::decode(bytes);
    lab::require(decoded.rate==w.rate && decoded.samples.size()==w.samples.size(),"WAV round trip header");
    for(std::size_t i=0;i<w.samples.size();++i)lab::require(std::fabs(decoded.samples[i]-w.samples[i])<=1./65536,"WAV quantization");
    const auto reject=[](const std::vector<uint8_t>& bad){bool failed=false;try{lab::decode(bad);}catch(const std::runtime_error&){failed=true;}lab::require(failed,"Malformed WAV accepted");};
    for(int which=0;which<8;++which){auto bad=bytes;
        switch(which){case 0:bad.pop_back();break;case 1:bad[0]='X';break;case 2:bad[20]=3;break;
        case 3:bad[22]=2;break;case 4:bad[34]=24;break;case 5:bad[24]=1;break;case 6:bad[40]=255;break;case 7:bad[32]=4;break;}
        reject(bad);
    }
    bool failed=false;try{lab::encode({48000,{NAN}});}catch(const std::runtime_error&){failed=true;}lab::require(failed,"Nonfinite WAV accepted");
    std::cout<<"Drive lab reference max error "<<err<<"; alias reduction dB:";
    for(const auto& row:a)std::cout<<' '<<row.one-row.four;
    std::cout<<"; low/high THD+N dBc "<<voice.lowThd<<'/'<<voice.highThd;
    std::cout<<"; WAV/FFT checks passed\n";
}
void audition(const std::filesystem::path& output,const std::filesystem::path& input) {
    lab::Wave wave=input.empty()?synthetic():lab::read(input);
    const std::string kind=input.empty()?"SYNTHETIC guitar-like plucks/strums; not recorded guitar":"USER-PROVIDED mono PCM16; recording provenance not verified";
    const std::array<std::string,5> names{"00_dry","01_low_drive","02_high_drive","03_high_dark","04_high_negative_bias"};
    // Refuse aliases/hard links to the original input before writing any result.
    if (!input.empty()) {
        std::vector<std::filesystem::path> destinations{"renders.csv","measurements.json","LISTEN.txt"};
        for (const auto& name : names) destinations.emplace_back(name+".wav");
        for (const auto& name : destinations) {
            const auto path=output/name;
            lab::require(std::filesystem::weakly_canonical(path)!=std::filesystem::canonical(input)
                && (!std::filesystem::exists(path)||!std::filesystem::equivalent(path,input)),
                "Output would overwrite the input recording; choose a separate directory");
        }
    }
    const std::array<DriveParameters,5> params{{{0,0,0,0},{.05f,.65f,.25f,.6f},{.9f,.65f,.25f,.6f},{.9f,.05f,.25f,.6f},{.9f,.65f,-.8f,.6f}}};
    std::array<std::vector<float>,5> signals; signals[0]=wave.samples;
    for(std::size_t i=1;i<signals.size();++i)signals[i]=render(wave.samples,static_cast<float>(wave.rate),params[i]);
    double target=std::pow(10.,-22./20);
    for(const auto& x:signals){const auto s=stats(x);if(s.peak>0)target=std::min(target,.89*s.rms/s.peak);}
    std::filesystem::create_directories(output);
    std::ofstream csv(output/"renders.csv"); lab::require(bool(csv),"Cannot create render manifest");csv<<std::setprecision(10);
    csv<<"# "<<kind<<"\n# rate="<<wave.rate<<", frames="<<wave.samples.size()<<", nominal_FIR_delay=48_samples; IIR_group_delay_additional\n";
    csv<<"file,drive,tone,bias,kernel_level,raw_rms_dbfs,post_gain,final_rms_dbfs,final_peak_dbfs,final_mean\n";
    for(std::size_t i=0;i<signals.size();++i){
        const auto raw=stats(signals[i]);const double gain=raw.rms>0?target/raw.rms:1.;
        for(auto& x:signals[i])x=static_cast<float>(x*gain);
        const auto final=stats(signals[i]); lab::require(final.peak<=.890001,"Normalization peak overflow");
        lab::write(output/(names[i]+".wav"),{wave.rate,signals[i]});
        const auto p=params[i]; csv<<names[i]<<".wav,"<<p.drive<<','<<p.tone<<','<<p.bias<<','<<p.level<<','<<db(raw.rms)<<','<<gain<<','<<db(final.rms)<<','<<db(final.peak)<<','<<final.mean<<'\n';
    }
    const auto aliases=aliasMeasurements(); const double error=referenceError(); const auto voice=voicingMeasurements();
    std::ofstream metrics(output/"measurements.json"); lab::require(bool(metrics),"Cannot create measurement report");metrics<<std::setprecision(10);
    metrics<<"{\n  \"measurement_rate_hz\":48000,\"fft_length\":8192,\"input_amplitude\":0.4,\"drive\":0.85,\"tone\":1,\"bias\":0.5,\"level\":0.6,\n  \"exact_solver_1x_max_absolute_error\":"<<error<<",\n  \"kernel_instance_bytes_host\":"<<sizeof(Drive)<<",\n";
    metrics<<"  \"voicing_1001_953125hz_amplitude_0_1\":{\"low_drive_0_05_thdn_dbc\":"<<voice.lowThd<<",\"high_drive_0_9_thdn_dbc\":"<<voice.highThd<<",\"symmetric_second_dbc\":"<<voice.symmetricSecond<<",\"bias_0_8_second_dbc\":"<<voice.biasedSecond<<"},\n";
    metrics<<"  \"bright_minus_dark_db_at_5876_953125hz_amplitude_0_005\":"<<voice.toneDifference<<",\n  \"alias_fixtures\":[\n";
    for(std::size_t i=0;i<aliases.size();++i){const auto a=aliases[i];metrics<<"    {\"frequency_hz\":"<<a.bin*48000./nfft<<",\"alias_1x_dbc\":"<<a.one<<",\"alias_4x_dbc\":"<<a.four<<",\"reduction_db\":"<<a.one-a.four<<'}'<<(i+1<aliases.size()?",":"")<<'\n';}
    metrics<<"  ]\n}\n";
    std::ofstream readme(output/"LISTEN.txt");lab::require(bool(readme),"Cannot create listening notes");
    readme<<kind<<"\nAll WAVs are mono PCM16 at "<<wave.rate<<" Hz.\nStart with 00_dry, compare 01_low_drive and 02_high_drive; then 03_high_dark and 04_high_negative_bias.\nWhole-file RMS matched to "<<db(target)<<" dBFS or lower for shared peak headroom; no LUFS/perceptual match claim.\nExact parameters/gains/levels in renders.csv. Silent input is -300 dBFS sentinel.\nNo latency compensation; 48-sample FIR delay plus frequency-dependent IIR delay.\nOptional input truncates any tail at its original length; include trailing silence in real recordings.\nSynthetic material and measurements are not founder listening approval, real-guitar audition, hardware proof or pedal-grade acceptance.\n";
    csv.flush(); metrics.flush(); readme.flush();
    lab::require(bool(csv)&&bool(metrics)&&bool(readme),"Cannot finish lab reports");
    std::cout<<"Created "<<signals.size()<<" audition WAVs in "<<output<<"\n"<<kind<<'\n';
}
}
int main(int argc,char** argv){try{
    std::filesystem::path output="drive-auditions",input;
    if(argc==2&&std::string(argv[1])=="--self-test"){selfTest();return 0;}
    for(int i=1;i<argc;++i){const std::string arg=argv[i];
        if((arg=="--output"||arg=="--input")&&i+1<argc){(arg=="--output"?output:input)=argv[++i];}
        else throw std::runtime_error("Usage: parable_drive_lab [--output DIR] [--input mono-pcm16.wav] | --self-test");
    }
    audition(output,input);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
