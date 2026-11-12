// Host Soundforge runner; actual native Spring is the only audio implementation.
#include "parable_spring.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
using parable::dsp::Spring;
namespace {
constexpr double pi=3.14159265358979323846;
// Unchanged owned seven-line FFT from rat_measure.h (no circuit dependency).
void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
struct Run {
    std::vector<float> audio;uint32_t impacts=0,limits=0,faults=0,clears=0;
    double restitution=0,projection=0,maxHFgain=0,maxEnergy=0,maxPenetration=0,maxBalance=0,discarded=0,us=0;
};
Run render(const lab::Wave& in,bool fault,const char* kind,std::ofstream* trace=nullptr) {
    Spring s;lab::require(s.prepare(float(in.rate),{fault,1,1}),"Spring prepare failed");
    Run r;r.audio.resize(in.samples.size());
    for(size_t n=0;n<in.samples.size();++n) {
        r.audio[n]=s.process(in.samples[n]);const auto& p=s.probe();
        r.restitution+=p.restitutionLossJ;r.projection+=p.projectionLossJ;r.discarded+=p.tailDiscardedJ;
        r.maxHFgain=std::max(r.maxHFgain,p.maximumImpactHighGainJ);r.maxEnergy=std::max(r.maxEnergy,p.energyEndJ);
        r.maxPenetration=std::max(r.maxPenetration,p.penetrationM);r.maxBalance=std::max(r.maxBalance,std::abs(p.balanceErrorJ));
        if(trace&&(n%128==0||p.impacts||p.tailDiscardedJ))
            *trace<<kind<<','<<fault<<','<<in.rate<<','<<n<<','<<double(n+1)/in.rate<<','<<in.samples[n]<<','<<p.displacementBeforeM<<','<<p.displacementAfterM<<','
              <<p.contactVelocityBefore<<','<<p.contactVelocityAfter<<','<<p.impacts<<','<<p.projections<<','<<p.penetrationM<<','<<p.energyStartJ<<','<<p.energyEndJ<<','
              <<p.inputWorkJ<<','<<p.dampingLossJ<<','<<p.restitutionLossJ<<','<<p.projectionLossJ<<','<<p.maximumImpactHighGainJ<<','<<p.highEnergyJ<<','<<p.balanceErrorJ<<','<<p.tailDiscardedJ<<'\n';
    }
    r.impacts=s.impacts();r.limits=s.inputLimits();r.faults=s.numericalFaults();r.clears=s.tailClears();
    lab::require(!r.faults&&!r.limits&&r.maxBalance<1e-18,"Normal evidence used containment/port limit or failed energy balance");
    lab::require(!fault?r.impacts==0:true,"Healthy support was contacted");
    // Separate full-processor timing pass includes stores, excludes trace/statistics.
    s.reset();std::vector<float> timed(in.samples.size());auto start=std::chrono::steady_clock::now();
    for(size_t n=0;n<timed.size();++n)timed[n]=s.process(in.samples[n]);
    r.us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(timed==r.audio,"Reset/repeat changed audio");return r;
}
void spectrum(std::ofstream& csv,const char* kind,unsigned rate,const std::vector<float>& signal,size_t start) {
    constexpr size_t size=8192;std::vector<std::complex<double>> bins(size);double sum=0;
    for(size_t n=0;n<size;++n){const double w=.5-.5*std::cos(2*pi*n/(size-1));sum+=w;bins[n]=signal[start+n]*w;}
    fft(bins);for(size_t n=0;n<=size/2;++n)csv<<kind<<','<<rate<<','<<n*double(rate)/size<<','<<std::abs(bins[n])*(n==0||n==size/2?1:2)/sum<<','<<start<<','<<size<<'\n';
}
void alias(const std::filesystem::path& path) {
    std::ofstream csv(path);lab::require(bool(csv),"Cannot create alias audit");
    csv<<std::setprecision(12)<<"fault,rate_hz,probe,window_frames,HF_above_16k_power_fraction,window_rms,impacts\n";
    // 32k Nyquist in fine-rate output: exposes impact bandwidth, not a claimed
    // measured folded-alias rejection. Compare rate-dependent events separately.
    for(bool fault:{false,true})for(unsigned rate:{32000u,48000u,96000u}) {
        Spring s;lab::require(s.prepare(float(rate),{fault,1,1}),"Alias prepare failed");
        constexpr size_t size=16384;std::vector<std::complex<double>> bins(size);double energy=0;
        for(size_t n=0;n<size;++n){const auto y=s.process(n<rate/200?float(.8*std::sin(pi*(double(n)+.5)/rate/.005)):0);
            const double w=.5-.5*std::cos(2*pi*n/(size-1));bins[n]=y*w;energy+=double(y)*y;}
        fft(bins);double total=0,above=0;for(size_t n=1;n<size/2;++n){const double p=std::norm(bins[n]);total+=p;if(n*double(rate)/size>16000)above+=p;}
        csv<<fault<<','<<rate<<",strong_half_sine,"<<size<<','<<(total?above/total:0)<<','<<std::sqrt(energy/size)<<','<<s.impacts()<<'\n';
        lab::require(s.numericalFaults()==0&&s.inputLimits()==0,"Alias probe contained");
    }
    csv.flush();lab::require(bool(csv),"Cannot finish alias audit");
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out) {
    auto music=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    lab::require(music.samples.size()>=music.rate&&music.samples.size()<=24*music.rate,"Input must be 1-24 seconds (six-second tail appended)");
    const auto original=soundforge::stats(music.samples);lab::require(original.peak>0,"Cannot calibrate silence");
    std::vector<std::string> names={"00_dry_raw.wav","01_healthy_raw.wav","02_contact_raw.wav","03_dry_matched.wav","04_healthy_matched.wav","05_contact_matched.wav"};
    for(const char* p:{"weak","strong","impulse"})for(const char* s:{"input","healthy","contact"})names.push_back(std::string(p)+"_"+s+"_raw.wav");
    for(const char* p:{"weak","strong","impulse"})for(const char* s:{"healthy","contact"})names.push_back(std::string(p)+"_"+s+"_common64.wav");
    auto paths=names;for(const char* p:{"renders.csv","energy.csv","spectrum.csv","alias.csv","comparison.csv","settings.json","LISTEN.txt"})paths.push_back(p);
    if(!input.empty())for(const auto& name:paths){const auto p=out/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite input");}
    const double calibration=.8/original.peak;for(auto& x:music.samples)x=float(x*calibration);
    music.samples.resize(music.samples.size()+6*music.rate,0);std::filesystem::create_directories(out);
    std::ofstream trace(out/"energy.csv"),levels(out/"renders.csv"),spectra(out/"spectrum.csv"),comparison(out/"comparison.csv");
    lab::require(bool(trace)&&bool(levels)&&bool(spectra)&&bool(comparison),"Cannot create receipts");
    trace<<std::setprecision(15)<<"probe,fault,rate_hz,frame,end_s,input,peak_pre_displacement_m,last_post_displacement_m,last_contact_u_before_m_s,last_contact_u_after_m_s,impacts,projections,penetration_m,energy_start_J,energy_end_J,input_work_J,material_loss_J,restitution_loss_J,projection_loss_J,maximum_impact_HF_kinetic_gain_J,HF_energy_J,balance_error_J,tail_discarded_J\n";
    levels<<std::setprecision(12)<<"file,source_kind,rate_hz,frames,input_gain,offline_gain,raw_rms,raw_peak,raw_crest_db,final_rms,final_peak,final_dc,final_crest_db,clipped_samples,impacts,port_limits,numerical_faults,tail_clears,peak_energy_J,restitution_J,projection_J,peak_impact_HF_gain_J,peak_penetration_m,peak_ledger_error_J,tail_discarded_J,host_processor_us\n";
    spectra<<std::setprecision(12)<<"signal,rate_hz,frequency_hz,peak_amplitude,window_start_frame,hann_frames\n";
    auto h=render(music,false,"guitar",&trace),f=render(music,true,"guitar",&trace);
    const std::vector<std::vector<float>> signals={music.samples,h.audio,f.audio};
    const double target=soundforge::rmsTarget(signals,std::pow(10.,-23./20),.89);
    auto write=[&](const std::string& name,const lab::Wave& source,const std::vector<float>& raw,const Run* run,const char* kind,double inputGain,double gain) {
        auto signal=raw;for(auto& x:signal)x=float(x*gain);const auto a=soundforge::stats(raw),b=soundforge::stats(signal);
        lab::require(b.peak<.999,"Clipped listening audio");lab::write(out/name,{source.rate,signal});
        Run zero;const auto& r=run?*run:zero;
        levels<<name<<','<<kind<<','<<source.rate<<','<<signal.size()<<','<<inputGain<<','<<gain<<','<<a.rms<<','<<a.peak<<','<<soundforge::db(a.peak/std::max(a.rms,1e-30))<<','<<b.rms<<','<<b.peak<<','<<b.mean<<','<<soundforge::db(b.peak/std::max(b.rms,1e-30))<<",0,"<<r.impacts<<','<<r.limits<<','<<r.faults<<','<<r.clears<<','<<r.maxEnergy<<','<<r.restitution<<','<<r.projection<<','<<r.maxHFgain<<','<<r.maxPenetration<<','<<r.maxBalance<<','<<r.discarded<<','<<r.us<<'\n';
    };
    for(unsigned n=0;n<6;++n){const auto& raw=signals[n%3];write(names[n],music,raw,n%3==0?nullptr:n%3==1?&h:&f,input.empty()?"owned-synthetic-plucks":"external-DI",calibration,n<3?1:soundforge::rmsGain(soundforge::stats(raw),target));}
    comparison<<std::setprecision(12)<<"probe,healthy_impacts,fault_impacts,raw_difference_rms,max_impact_HF_gain_J,restitution_J,projection_J,peak_penetration_m\n";
    auto compare=[&](const char* kind,const Run& a,const Run& b){double delta=0;for(size_t n=0;n<a.audio.size();++n)delta+=std::pow(double(a.audio[n])-b.audio[n],2);comparison<<kind<<','<<a.impacts<<','<<b.impacts<<','<<std::sqrt(delta/a.audio.size())<<','<<b.maxHFgain<<','<<b.restitution<<','<<b.projection<<','<<b.maxPenetration<<'\n';};
    compare("music",h,f);spectrum(spectra,"guitar_healthy",music.rate,h.audio,music.rate);spectrum(spectra,"guitar_contact",music.rate,f.audio,music.rate);
    for(unsigned n=0;n<3;++n) {
        const char* kind=n==0?"weak":n==1?"strong":"impulse";lab::Wave p;p.rate=48000;p.samples.resize(6*p.rate+240,0);
        if(n==2)p.samples[0]=.8f;
        else for(unsigned i=0;i<240;++i)p.samples[i]=float((n==0?.05:.8)*std::sin(pi*(i+.5)/240));
        const auto a=render(p,false,kind,&trace),b=render(p,true,kind,&trace);
        if(n==0)lab::require(a.audio==b.audio&&b.impacts==0,"Weak pluck contacted");
        if(n==1)lab::require(b.impacts>10&&b.maxHFgain>1e-12&&b.projection<.3*b.restitution,"Strong contact mechanism failed");
        compare(kind,a,b);write(names[6+n*3],p,p.samples,nullptr,"owned-probe",1,1);write(names[7+n*3],p,a.audio,&a,"owned-probe",1,1);write(names[8+n*3],p,b.audio,&b,"owned-probe",1,1);
        write(names[15+n*2],p,a.audio,&a,"owned-probe",1,64);write(names[16+n*2],p,b.audio,&b,"owned-probe",1,64);
        spectrum(spectra,(std::string(kind)+"_healthy").c_str(),p.rate,a.audio,0);spectrum(spectra,(std::string(kind)+"_contact").c_str(),p.rate,b.audio,0);
    }
    std::ofstream settings(out/"settings.json"),notes(out/"LISTEN.txt");lab::require(bool(settings)&&bool(notes),"Cannot create settings");
    settings<<std::setprecision(12)<<"{\"standalone_not_canonical\":true,\"input_peak\":0.8,\"software_input_gain\":"<<calibration<<",\"mix\":1,\"wet_return\":1,\"modes\":192,\"substeps\":2,\"FIR_taps\":193,\"FIR_delay_base_frames\":48,\"healthy_gap_m\":0.005,\"fault_gap_m\":0.000005,\"restitution\":0.55,\"energy_cap_J\":0.000002,\"stored_tail_s\":6,\"processor_bytes\":"<<sizeof(Spring)<<"}\n";
    notes<<"SF-008 experimental two-path modal spring surrogate; no measured tank or founder-approved correspondence.\n"
         <<"Compare 04_healthy_matched.wav / 05_contact_matched.wav. 00 dry / 01 healthy / 02 fault are RAW common gain 1. RMS matching is separate, not LUFS; gains, crest and levels are in renders.csv. Pure wet return, no hidden noise or crash sample. Raw native reverb is quiet because of the conservative energy/pickup bound.\n"
         <<"Same full permitted DI, input peak .8 NORMALIZED, not voltage; six-second silent tail, no resampling. Original/card/notices are in the artifact.\n"
         <<"Fault only moves ONE positive support boundary on path0: 5mm -> 5um. Path1 unaffected. Numerical projection loss is separately reported; rigid restitution e=.55 loses total energy while redistributing modal HF kinetic energy. No measured suspension clearance.\n"
         <<"Weak/strong are OWNED 5ms half-sine forces at peak .05/.8. Impulse is one .8 frame, not a physical Dirac or DI guitar. Probe responses retain RAW native gain; *_common64.wav ALSO provide the same fixed offline gain64 for every weak/strong/impulse processed signal, preserving their relative levels. energy.csv decimates 128 frames plus every impact/clear; each row aggregates TWO substeps: peak displacement/penetration, last-contact velocity pair, summed frame energy terms.\n"
         <<"Continuous helical coupling, gravity, shell, coil/transducer electronics and noise omitted. Material damping is passive split attenuation; unilateral rigid stop and overshoot projection are approximations. alias.csv reports fine-output power above16k in fixed16384-frame windows (different physical durations), NOT a folded-alias rejection floor or convergence comparison.\n"
         <<"Return bounded by energy/pickup and FIR L1; normal renders require ZERO port limiting or numerical containment. Six-second clear records discarded energy.\n"
         <<"Host timed reset pass includes the full processor/stores, excludes prepare/IO/analysis. No Daisy runtime, boot, analog IO or subjective musical approval measured.\n";
    alias(out/"alias.csv");for(auto* s:{&trace,&levels,&spectra,&comparison,&settings,&notes}){s->flush();lab::require(bool(*s),"Cannot finish evidence");}
    std::cout<<"Created21 spring WAVs and passive-contact evidence in "<<out<<"; guitar impacts="<<f.impacts<<'\n';
}
}
int main(int argc,char** argv){try{
    std::filesystem::path input,out;bool seenInput=false,seenOutput=false;
    for(int i=1;i<argc;++i){std::string key=argv[i];lab::require((key=="--input"||key=="--output")&&i+1<argc,"Use --output DIR [--input mono-pcm16.wav]");std::string v=argv[++i];lab::require(!v.empty(),"Empty path");bool& seen=key=="--input"?seenInput:seenOutput;lab::require(!seen,"Duplicate argument");seen=true;(key=="--input"?input:out)=v;}
    lab::require(seenOutput,"Explicit output required");audition(input,out);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
