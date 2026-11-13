// Host audio/evidence runner; only parable::dsp::Muff creates processed audio.
#include "parable_muff.h"
#include "muff_stimulus.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
using parable::dsp::Muff;
namespace {
constexpr double pi=mufflab::pi;
// Unchanged owned seven-line FFT from rat_measure.h, no RAT circuit dependency.
void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
struct Run {std::vector<float> y;std::vector<std::array<double,8>> windows;double us=0,meanEvaluations=0;uint32_t evaluations=0,solves=0,maximum=0,failures=0,clamps=0;};
Run render(const lab::Wave& source,bool fault,const char* kind,std::ofstream& trace,bool probe=false) {
    Muff m;lab::require(m.prepare(float(source.rate),{fault,.75f,.5f,.5f}),"Muff prepare failed");
    Run r;r.y.resize(source.samples.size());
    std::array<std::array<double,7>,7> sums{};std::array<unsigned,7> counts{};
    const double times[]={.09,.13,.18,.3,.35,.45,.6};
    for(size_t n=0;n<r.y.size();++n) {
        r.y[n]=m.process(source.samples[n]);
        if(n%128==0) {
            trace<<kind<<','<<fault<<','<<source.rate<<','<<n<<','<<(double(n)+.75)/source.rate<<','<<source.samples[n]<<','<<r.y[n];
            for(auto v:m.nodes())trace<<','<<v;
            trace<<','<<m.capacitorVolts(6)<<','<<m.capacitorVolts(12)<<','<<m.diodeAmps(0)<<','<<m.diodeAmps(1)<<','<<m.toneVolts()<<','<<m.jackVolts()<<','<<m.inputAmps()<<','<<m.residualVolts()<<','<<m.failures()<<'\n';
        }
        if(probe)for(unsigned k=0;k<7;++k)if(double(n)/source.rate>=times[k]&&double(n)/source.rate<times[k]+.02) {
            const auto& s=m.nodes();const double values[]={double(r.y[n])*r.y[n],s[Muff::B2],s[Muff::C2],s[Muff::E2],m.capacitorVolts(6),m.capacitorVolts(12),m.jackVolts()*m.jackVolts()};
            for(unsigned j=0;j<7;++j)sums[k][j]+=values[j];
            ++counts[k];
        }
    }
    if(probe)for(unsigned k=0;k<7;++k){std::array<double,8> row{};row[0]=times[k];for(unsigned j=0;j<7;++j)row[j+1]=sums[k][j]/counts[k];row[1]=std::sqrt(row[1]);row[7]=std::sqrt(row[7]);r.windows.push_back(row);}
    r.evaluations=m.evaluations();r.solves=m.solves();r.maximum=m.maximumEvaluations();r.failures=m.failures();r.clamps=m.outputClamps();r.meanEvaluations=double(r.evaluations)/r.solves;
    lab::require(!r.failures&&!r.clamps&&!m.inputClamps()&&!m.invalidInputs(),"Refuse solver/output/input containment as a component fault");
    m.reset();std::vector<float> timed(r.y.size());const auto start=std::chrono::steady_clock::now();
    for(size_t n=0;n<timed.size();++n)timed[n]=m.process(source.samples[n]);
    r.us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(timed==r.y,"Reset/repeat changed audio");return r;
}
void spectrum(std::ofstream& csv,const char* label,unsigned rate,const std::vector<float>& signal,size_t start) {
    constexpr size_t size=8192;std::vector<std::complex<double>> bins(size);double sum=0;
    for(size_t i=0;i<size;++i){const double w=.5-.5*std::cos(2*pi*i/(size-1));sum+=w;bins[i]=signal[start+i]*w;}
    fft(bins);for(size_t i=0;i<=size/2;++i)csv<<label<<','<<i*double(rate)/size<<','<<std::abs(bins[i])*(i==0||i==size/2?1:2)/sum<<','<<rate<<','<<start<<','<<size<<'\n';
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out) {
    auto music=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    lab::require(music.samples.size()>=music.rate&&music.samples.size()<=24*music.rate,"Input must be1-24seconds; two-second tail appended");
    const auto original=soundforge::stats(music.samples);lab::require(original.peak>0,"Cannot calibrate silence");
    std::vector<std::string> names={"00_dry_raw.wav","01_healthy_raw.wav","02_R15_high_raw.wav","03_dry_matched.wav","04_healthy_matched.wav","05_R15_high_matched.wav"};
    for(const char* kind:{"two_attack","probe_only","long_note"})for(const char* variant:{"input_raw","healthy_raw","R15_high_raw","healthy_common","R15_high_common"})names.push_back(std::string(kind)+"_"+variant+".wav");
    auto paths=names;for(const char* f:{"renders.csv","state.csv","spectrum.csv","recovery.csv","settings.json","LISTEN.txt"})paths.push_back(f);
    if(!input.empty())for(const auto& name:paths){const auto p=out/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite source");}
    const double calibration=.2/original.peak;for(auto& x:music.samples)x=float(x*calibration);music.samples.resize(music.samples.size()+2*music.rate,0);
    std::filesystem::create_directories(out);std::ofstream trace(out/"state.csv"),receipt(out/"renders.csv"),spectra(out/"spectrum.csv"),recovery(out/"recovery.csv");
    lab::require(bool(trace)&&bool(receipt)&&bool(spectra)&&bool(recovery),"Cannot create receipts");
    trace<<std::setprecision(15)<<"source,fault,rate_hz,frame,last_phase3_sample_s,dry_input,filtered_normalized_output,b4,c4,e4,b3,c3,e3,b2,c2,e2,b1,c1,e1,sustain_w,low,high,d3,d2,C7_B2_D2_V,C13_C3_i2_V,pair_Q3_A,pair_Q2_A,tone_V,jack_V,input_A,port_residual_V,solver_failures\n";
    receipt<<std::setprecision(12)<<"file,source_kind,rate_hz,frames,input_gain,offline_gain,raw_rms,raw_peak,raw_crest_db,final_rms,final_peak,final_dc,final_crest_db,clipped_samples,solver_failures,output_containment,mean_evaluations,max_evaluations,host_processor_us\n";
    spectra<<std::setprecision(12)<<"signal,frequency_hz,peak_amplitude,rate_hz,window_start_frame,hann_frames\n";
    recovery<<std::setprecision(12)<<"fault,prior_attacks,start_s,end_s,normalized_rms,mean_B2_V,mean_C2_V,mean_E2_V,mean_C7_V,mean_C13_V,last_phase3_jack_rms_V\n";
    const auto h=render(music,false,"music",trace),f=render(music,true,"music",trace);
    const std::vector<std::vector<float>> signals={music.samples,h.y,f.y};
    double target=soundforge::rmsTarget(signals,std::pow(10.,-23./20),.89);
    // Explicit ceiling prevents turning a near-muted failed hypothesis into loud noise.
    for(const auto& s:signals)target=std::min(target,16*soundforge::stats(s).rms);
    auto write=[&](const std::string& name,const lab::Wave& source,const std::vector<float>& raw,const Run* run,const char* kind,double inputGain,double gain) {
        auto y=raw;for(auto& v:y)v=float(v*gain);const auto a=soundforge::stats(raw),b=soundforge::stats(y);lab::require(b.peak<.999,"Clipped listening audio");lab::write(out/name,{source.rate,y});
        Run zero;const auto& r=run?*run:zero;
        receipt<<name<<','<<kind<<','<<source.rate<<','<<y.size()<<','<<inputGain<<','<<gain<<','<<a.rms<<','<<a.peak<<','<<soundforge::db(a.peak/std::max(a.rms,1e-30))<<','<<b.rms<<','<<b.peak<<','<<b.mean<<','<<soundforge::db(b.peak/std::max(b.rms,1e-30))<<",0,"<<r.failures<<','<<r.clamps<<','<<r.meanEvaluations<<','<<r.maximum<<','<<r.us<<'\n';
    };
    for(unsigned n=0;n<6;++n)write(names[n],music,signals[n%3],n%3==0?nullptr:n%3==1?&h:&f,input.empty()?"owned-synthetic":"caller-provided-PCM16",calibration,n<3?1:soundforge::rmsGain(soundforge::stats(signals[n%3]),target));
    spectrum(spectra,"music_dry",music.rate,music.samples,music.rate);spectrum(spectra,"music_healthy",music.rate,h.y,music.rate);spectrum(spectra,"music_R15_high",music.rate,f.y,music.rate);
    for(unsigned kind=0;kind<3;++kind) {
        lab::Wave p;p.rate=48000;p.samples.resize(kind==2?2*p.rate:size_t(1.2*p.rate));
        for(size_t n=0;n<p.samples.size();++n)p.samples[n]=kind==2?mufflab::note(double(n)/p.rate):mufflab::history(double(n)/p.rate,kind==0);
        const char* label=kind==0?"two_attack":kind==1?"probe_only":"long_note";
        const auto a=render(p,false,label,trace,kind<2),b=render(p,true,label,trace,kind<2);
        const double common=std::min(8.,.89/std::max(soundforge::stats(a.y).peak,soundforge::stats(b.y).peak));
        write(names[6+kind*5],p,p.samples,nullptr,"owned-controlled",1,1);write(names[7+kind*5],p,a.y,&a,"owned-controlled",1,1);write(names[8+kind*5],p,b.y,&b,"owned-controlled",1,1);
        write(names[9+kind*5],p,a.y,&a,"owned-controlled",1,common);write(names[10+kind*5],p,b.y,&b,"owned-controlled",1,common);
        if(kind<2)for(unsigned fault=0;fault<2;++fault)for(const auto& row:(fault?b:a).windows){recovery<<fault<<','<<(kind==0)<<','<<row[0]<<','<<row[0]+.02;for(unsigned j=1;j<8;++j)recovery<<','<<row[j];recovery<<'\n';}
        spectrum(spectra,(std::string(label)+"_healthy").c_str(),p.rate,a.y,0);spectrum(spectra,(std::string(label)+"_R15_high").c_str(),p.rate,b.y,0);
    }
    std::ofstream settings(out/"settings.json"),notes(out/"LISTEN.txt");lab::require(bool(settings)&&bool(notes),"Cannot write settings");
    settings<<std::setprecision(12)<<"{\"standalone_not_canonical\":true,\"source_kind\":\""<<(input.empty()?"owned-synthetic":"caller-provided-PCM16")<<"\",\"software_input_gain\":"<<calibration<<",\"input_peak_model_V\":0.2,\"source_ohms\":1000,\"load_ohms\":1000000,\"supply_V\":9,\"sustain\":0.75,\"tone\":0.5,\"volume\":0.5,\"second_clip_reference\":\"Q2\",\"healthy_R15_ohms\":470000,\"fault_R15_ohms\":2200000,\"output_V_per_unit\":9,\"oversampling\":4,\"FIR_taps\":193,\"nominal_FIR_delay_base_frames\":48,\"maximum_offline_music_gain\":16,\"processor_bytes\":"<<sizeof(Muff)<<",\"post_overload_suppression_proven\":false}\n";
    notes<<"SF-009 experimental ElectroSmash American V3 four-transistor fuzz. Source drawing numbering:Q4 input, Q3 first clipping, Q2 SECOND clipping, Q1 recovery. No measured EHX-unit or founder approval.\n"
         <<"Compare04_healthy_matched.wav /05_R15_high_matched.wav;00dry/01healthy/02fault RAW commongain1 remain. Whole-file RMS matching, not LUFS, capped at16x gain. Actual levels/crest/normalization inrenders.csv. No near-silence laundering or fault-specific native gain.\n"
         <<"New primary DI source/rights/original and deterministic stereo24->mono16 conversion are supplied by CI, not inferred for arbitrary --input files. Input peak .2 MODEL volts, not measured pickup/Daisy voltage; same input/settings, two-second appended tail, no resampling.\n"
         <<"Only Q2 R15 changes470k->2.2M. The bias nearly cuts off. IMPORTANT: current two-attack experiment shows temporary PRIMING/ENHANCEMENT of the subsequent weak probe, not proven post-overload suppression. Intended choking/recovery hypothesis remains unfulfilled; no added gate or dropout. recovery.csv compares identical low probes with/without prior attacks.\n"
         <<"two_attack/probe_only/long_note files are OWNED SYNTHETIC evidence, not guitar. *_common.wav apply the same headroom-limited gain to healthy/fault within each stimulus, preserving their relative levels. Long note exposes altered weak sustain/decay; founder judges usefulness.\n"
         <<"Full loaded coupling, emitter/bias relationships, diode-feedback DC-blocking caps and tone stack are solved together. Reduced nominal Ebers-Moll silicon; all13 external caps. No Early/high-injection/intrinsic charge/noise/supply dynamics.\n"
         <<"state.csv decimates128frames; node/current/cap values are LAST internalphase3, emitted filtered WAV uses phase0 decimation. Time column is last-phase sample instant (frame+.75)/rate; do not treat node traces as latency-aligned output samples. Diode currents are actual net pair current, C7 sign B2-D2, C13 across capacitor before R12.\n"
         <<"Every normal render requires ZERO solver fallback or output/input containment. Physical domain/iteration bounds protect arbitrary invalid/stress input, not an analog fault. Bias capacitor history may outlast the saved2s silent tail; no artificial tail clear.\n"
         <<"Host timing is a separate reset/repeat full-process pass excluding prepare/trace/IO/analysis. No measured Daisy deadline, boot, audio IO or subjective musical approval.\n";
    for(auto* s:{&trace,&receipt,&spectra,&recovery,&settings,&notes}){s->flush();lab::require(bool(*s),"Cannot finish receipts");}
    std::cout<<"Created21 Muff WAVs and true electrical/history receipts in "<<out<<"; hypothesis: priming observed, suppression NOT established\n";
}
}
int main(int argc,char** argv){try{
    std::filesystem::path input,out;bool si=false,so=false;
    for(int i=1;i<argc;++i){const std::string k=argv[i];lab::require((k=="--input"||k=="--output")&&i+1<argc,"Use --output DIR [--input mono-pcm16.wav]");const std::string v=argv[++i];lab::require(!v.empty(),"Empty path");bool& seen=k=="--input"?si:so;lab::require(!seen,"Duplicate argument");seen=true;(k=="--input"?input:out)=v;}
    lab::require(so,"Explicit output required");audition(input,out);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
