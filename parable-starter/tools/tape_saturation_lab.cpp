// Actual native SF012 listening/measurement runner. All allocation and file work
// stays here on the host. This is not a second waveshaper implementation.
#include "parable_tape_saturation.h"
#include "tape_saturation_stimulus.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
using parable::dsp::TapeSaturation;
namespace {
// Existing owned laboratory FFT, unchanged arithmetic.
void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*tapelab::pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
struct Files {std::ofstream levels,state,parcels,spectra,hist,performance;};
std::vector<float> render(const lab::Wave& x,bool weak,const std::string& label,Files& f) {
    TapeSaturation m;lab::require(m.prepare(float(x.rate),weak),"Tape preparation failed");std::vector<float> y(x.samples.size()),repeat(y.size());std::array<uint64_t,128> histogram{};
    const auto peak=std::size_t(std::max_element(x.samples.begin(),x.samples.end(),[](float a,float b){return std::abs(a)<std::abs(b);})-x.samples.begin());
    for(std::size_t n=0;n<y.size();++n){y[n]=m.process(x.samples[n]);const auto& p=m.probe();++histogram[std::clamp(int((p.remanence+1)*64),0,127)];
        if(n%128==0)f.state<<label<<','<<weak<<','<<x.rate<<','<<n<<','<<x.samples[n]<<','<<p.recordedVolts<<','<<p.remanence<<','<<p.flux_nWb_per_m<<','<<p.headRemanence<<','<<p.coilVolts<<','<<p.outputVolts<<'\n';
        if(n==peak)for(unsigned q=0;q<8;++q){parable::dsp::TapeParcelTrace trace;lab::require(m.traceParcel(q,trace),"Parcel trace failed");for(std::size_t j=0;j<trace.count;++j){const auto& t=trace.points[j];f.parcels<<label<<','<<weak<<','<<x.rate<<','<<n<<','<<q<<','<<j<<','<<t.relativeSeconds<<','<<t.field_A_per_m<<','<<t.magnetization;for(double p:t.boundary_A_per_m)f.parcels<<','<<p;f.parcels<<'\n';}}
    }
    lab::require(!m.failures()&&!m.invalidInputs()&&!m.inputClamps()&&!m.outputClamps(),"Containment cannot be rendered as tape sound");
    for(unsigned b=0;b<128;++b)f.hist<<label<<','<<weak<<','<<(-1+b/64.)<<','<<(-1+(b+1)/64.)<<','<<histogram[b]<<'\n';
    m.reset();const auto start=std::chrono::steady_clock::now();for(std::size_t n=0;n<repeat.size();++n)repeat[n]=m.process(x.samples[n]);const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(repeat==y&&!m.failures()&&!m.inputClamps()&&!m.outputClamps(),"Exact reset/timed native repeat failed");
    f.performance<<label<<','<<weak<<','<<x.rate<<','<<y.size()<<','<<us<<','<<us/y.size()<<','<<sizeof(m)<<','<<m.parcels()<<'\n';
    constexpr unsigned size=8192;std::vector<std::complex<double>> a(size);const auto first=std::min<std::size_t>(x.rate,y.size()-size);double sum=0;
    for(unsigned n=0;n<size;++n){const double w=.5-.5*std::cos(2*tapelab::pi*n/(size-1));a[n]=y[first+n]*w;sum+=w;}fft(a);
    for(unsigned n=0;n<=size/2;++n)f.spectra<<label<<','<<weak<<','<<double(x.rate)*n/size<<','<<std::abs(a[n])*(n==0||n==size/2?1:2)/sum<<','<<x.rate<<','<<first<<','<<size<<'\n';
    return y;
}
void group(const lab::Wave& x,const std::string& label,double calibration,const std::filesystem::path& out,Files& f,bool rms) {
    std::vector<std::vector<float>> signals(3);signals[0]=x.samples;for(auto& a:signals[0])a/=8;signals[1]=render(x,false,label,f);signals[2]=render(x,true,label,f);
    std::array<soundforge::Stats,3> stats{};double common=16;for(unsigned j=0;j<3;++j){stats[j]=soundforge::stats(signals[j]);if(stats[j].rms>0)common=std::min(common,.1/stats[j].rms);if(stats[j].peak>0)common=std::min(common,.9/stats[j].peak);}
    double target=soundforge::rmsTarget(signals,.1,.9);for(const auto& s:stats)target=std::min(target,s.rms*16);
    for(unsigned mode=0;mode<(rms?3u:2u);++mode)for(unsigned j=0;j<3;++j){const std::string kind=j==0?"dry":j==1?"healthy":"weakbias",version=mode==0?"raw":mode==1?"common":"rms_matched",name=label+"_"+kind+"_"+version+".wav";
        const double gain=mode==0?1:mode==1?common:soundforge::rmsGain(stats[j],target);lab::require(gain>0&&gain<=16.000001,"Offline gain bound");lab::Wave y{x.rate,signals[j]};for(auto& a:y.samples)a=float(a*gain);const auto final=soundforge::stats(y.samples);lab::require(final.peak<.999,"Offline gain clips");lab::write(out/name,y);
        f.levels<<name<<','<<label<<','<<kind<<','<<x.rate<<','<<y.samples.size()<<','<<calibration<<','<<(j==2?.006:.012)<<','<<stats[j].rms<<','<<stats[j].peak<<','<<stats[j].mean<<','<<gain<<','<<final.rms<<','<<final.peak<<','<<final.mean<<",0,0,0\n";
    }
}
std::vector<std::string> names() {
    std::vector<std::string> result;for(const char* group:{"low","medium","hard"})for(const char* signal:{"dry","healthy","weakbias"})for(const char* mode:{"raw","common","rms_matched"})result.push_back(std::string(group)+"_"+signal+"_"+mode+".wav");
    for(const char* group:{"attacks","sustain","chord","two","sweep"})for(const char* signal:{"dry","healthy","weakbias"})for(const char* mode:{"raw","common"})result.push_back(std::string(group)+"_"+signal+"_"+mode+".wav");
    for(const char* name:{"renders.csv","state.csv","parcels.csv","spectrum.csv","histograms.csv","performance.csv","settings.json","LISTEN.txt"})result.push_back(name);
    return result;
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out) {
    auto original=input.empty()?soundforge::syntheticPlucks():lab::read(input);lab::require(original.samples.size()>=original.rate&&original.samples.size()<=28*original.rate,"Input must be1–28s");lab::require(original.rate==44100||original.rate==48000||original.rate==96000,"SF012 supports44.1/48/96k inputs");const auto stats=soundforge::stats(original.samples);lab::require(stats.peak>0,"Cannot calibrate silence");
    // Validate ALL output aliases before creating/truncating any file.
    if(!input.empty())for(const auto& name:names()){const auto p=out/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output aliases input");}
    std::filesystem::create_directories(out);Files f;f.levels.open(out/"renders.csv");f.state.open(out/"state.csv");f.parcels.open(out/"parcels.csv");f.spectra.open(out/"spectrum.csv");f.hist.open(out/"histograms.csv");f.performance.open(out/"performance.csv");for(auto* s:{&f.levels,&f.state,&f.parcels,&f.spectra,&f.hist,&f.performance}){lab::require(bool(*s),"Cannot create receipt");*s<<std::setprecision(16);}
    f.levels<<"file,group,signal,rate_hz,frames,input_gain_V_per_source_unit,bias_peak_A,raw_rms,raw_peak,raw_mean,offline_gain,final_rms,final_peak,final_mean,numerical_failures,input_clamps,output_clamps\n";
    f.state<<"group,weak,rate_hz,frame,input_V,last4x_record_V,last4x_M,last4x_flux_nWb_per_m,playback_M,coil_backward_difference_V,output_V\n";
    f.parcels<<"group,weak,rate_hz,frame,phase,event,relative_s,H_A_per_m,M";for(unsigned i=0;i<8;++i)f.parcels<<",p"<<i<<"_A_per_m";f.parcels<<'\n';
    f.spectra<<"group,weak,hz,amplitude,rate_hz,start_frame,hann_frames\n";f.hist<<"group,weak,M_low,M_high,count\n";f.performance<<"group,weak,rate_hz,frames,full_process_us,us_per_frame,state_bytes,internal_parcels\n";
    const std::array<std::string,3> labels={"low","medium","hard"};const std::array<double,3> volts={.05,.2,.8};
    for(unsigned g=0;g<3;++g){auto x=original;const double gain=volts[g]/stats.peak;for(auto& a:x.samples)a=float(a*gain);x.samples.resize(x.samples.size()+x.rate*3/20);group(x,labels[g],gain,out,f,true);}
    for(const std::string kind:{"attacks","sustain","chord","two","sweep"}){lab::Wave x{48000,{}};const unsigned length=kind=="sustain"?96000:kind=="attacks"?24000:57600;for(unsigned n=0;n<length;++n)x.samples.push_back(float(tapelab::stimulus(kind,double(n)/x.rate,kind=="sweep"?.2:.8)));x.samples.resize(x.samples.size()+7200);group(x,kind,1,out,f,false);}
    for(auto* s:{&f.levels,&f.state,&f.parcels,&f.spectra,&f.hist,&f.performance}){s->flush();lab::require(bool(*s),"Receipt write failed");}
    std::ofstream settings(out/"settings.json");settings<<"{\"experimental\":true,\"healthy_bias_peak_A\":0.012,\"weak_bias_peak_A\":0.006,\"bias_Hz\":100000,\"speed_m_per_s\":0.381,\"record_gap_m\":0.000007,\"playback_gap_m\":0.000003,\"coating_m\":0.000016,\"effective_record_depth_m\":0.0000083,\"record_lead_zero_s\":0.00005,\"record_lead_pole_s\":0.000005,\"playback_HP_Hz\":30,\"output_V_per_unit_M\":0.75,\"software_divisor\":8,\"oversampling\":4,\"FIR_taps\":193,\"pickup_taps\":257,\"phase_quadrature\":8,\"state_bytes\":"<<sizeof(TapeSaturation)<<",\"offline_gain_ceiling\":16,\"tail_s\":0.15,\"input_calibration_peak_V\":[0.05,0.2,0.8]}\n";
    std::ofstream listen(out/"LISTEN.txt");listen<<"SF012 EXPERIMENTAL magnetic tape saturation. Start medium_dry_common, medium_healthy_common, medium_weakbias_common; common gain retains fault loudness. Compare *_rms_matched to reduce RMS bias (individual gains, not perceptual matching). Then low and hard groups. Every group uses identical phrase/timing/calibration. Raw software Volts/8 files are deliberately quieter. No unexplained normalizer; all gains<=16 in renders.csv. Synthetic attacks/sustain/chord/two/sweep are separate, not recordings. Output latency is a few milliseconds (see measured report), no echo. Same material/transport/electronics; only bias-only current12->6mA. One-depth phenomenological material, not a fitted/measured deck. Founder listening/physical tape/target timing NOT established.\n";
    settings.flush();listen.flush();lab::require(bool(settings)&&bool(listen),"Settings/listening write failed");
}
void measurements(const std::filesystem::path& out) {
    std::filesystem::create_directories(out);
    std::ofstream stream(out/"native-material.csv"),tones(out/"tones.csv"),impulse(out/"impulse.csv"),memory(out/"history.csv");
    for(auto* f:{&stream,&tones,&impulse,&memory}){lab::require(bool(*f),"Measurement output failed");*f<<std::setprecision(17);}
    stream<<"base_rate_hz,kind,weak,frame,time_s,M\n";
    tones<<"rate_hz,weak,input_peak_V,hz,output_peak_fundamental_V,H2_projection_V,H3_projection_V,H5_projection_V,H7_projection_V,output_RMS_V,DC_V,peak_V,record_flux_RMS_nWb_per_m,record_flux_peak_nWb_per_m\n";
    impulse<<"rate_hz,weak,frame,input_V,output_V\n";memory<<"internal_frame,common_current_input_V,excited_history_M,erased_history_M\n";
    for(unsigned rate:{44100u,48000u,96000u})for(bool weak:{false,true}) {
        for(const std::string kind:{"tone","hf"}){TapeSaturation m;lab::require(m.prepare(float(rate),weak),"Material stream prepare");const unsigned internal=4*rate;const double amplitude=kind=="tone"?.2:.8;
            for(unsigned n=0;n<internal/25;++n){const double t=double(n)/internal;const auto y=m.materialReference(float(tapelab::stimulus(kind,t,amplitude)));if(n>=internal/50)stream<<rate<<','<<kind<<','<<weak<<','<<n-internal/50<<','<<t<<','<<y<<'\n';}
        }
        for(double a:{.005,.05,.13,.2,.8})for(double hz:{100.,1000.,8000.,16000.}) {
            TapeSaturation m;lab::require(m.prepare(float(rate),weak),"Tone prepare");std::array<std::complex<double>,5> sums{};double power=0,mean=0,peak=0,fluxPower=0,fluxPeak=0;unsigned count=0;
            for(unsigned n=0;n<rate/10;++n){const double t=double(n)/rate;const double y=8*m.process(float(a*std::sin(2*tapelab::pi*hz*t)));
                if(n>=rate/20){const unsigned harmonics[]={1,2,3,5,7};for(unsigned h=0;h<5;++h)sums[h]+=y*std::polar(1.,-2*tapelab::pi*hz*harmonics[h]*t);power+=y*y;mean+=y;peak=std::max(peak,std::abs(y));const double flux=m.probe().flux_nWb_per_m;fluxPower+=flux*flux;fluxPeak=std::max(fluxPeak,std::abs(flux));++count;}}
            lab::require(!m.failures()&&!m.inputClamps()&&!m.outputClamps(),"Tone containment");tones<<rate<<','<<weak<<','<<a<<','<<hz;for(const auto z:sums)tones<<','<<2*std::abs(z)/count;tones<<','<<std::sqrt(power/count)<<','<<mean/count<<','<<peak<<','<<std::sqrt(fluxPower/count)<<','<<fluxPeak<<'\n';
        }
        TapeSaturation m;lab::require(m.prepare(float(rate),weak),"Impulse prepare");for(unsigned n=0;n<rate/50;++n){const float x=n==0?.01f:0;impulse<<rate<<','<<weak<<','<<n<<','<<x<<','<<8*m.process(x)<<'\n';}
    }
    TapeSaturation excited,erased;lab::require(excited.prepare(48000)&&erased.prepare(48000),"History prepare");
    for(unsigned n=0;n<200;++n){const float x=n<100?.8f:0;const float a=excited.materialReference(x),b=erased.materialReference(0);if(n>=100)memory<<n<<",0,"<<a<<','<<b<<'\n';}
    for(auto* f:{&stream,&tones,&impulse,&memory}){f->flush();lab::require(bool(*f),"Measurement write failed");}
}
}
int main(int argc,char** argv)try {
    if(argc==3&&std::string(argv[1])=="--measure"){lab::require(std::string(argv[2]).size()>0,"Empty measure output");measurements(argv[2]);return 0;}
    std::filesystem::path input,out;bool haveInput=false,haveOutput=false;
    for(int i=1;i<argc;++i){const std::string arg=argv[i];lab::require(i+1<argc,"Missing option value");const std::string value=argv[++i];lab::require(!value.empty(),"Empty option value");if(arg=="--input"&&!haveInput){input=value;haveInput=true;}else if(arg=="--output"&&!haveOutput){out=value;haveOutput=true;}else throw std::runtime_error("Unknown/duplicate option");}
    lab::require(haveOutput,"Require --output DIR, optional --input MONO_PCM16.wav");audition(input,out);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
