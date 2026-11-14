// Host laboratory only; every musical output uses the actual native Klon.
#include "parable_klon.h"
#include "klon_stimulus.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
using parable::dsp::Klon;using parable::dsp::KlonControls;
namespace {
constexpr double pi=klonlab::pi;
// Owned seven-line FFT reused by the existing laboratories.
void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
struct Receipts {std::ofstream trace,stages,hist,levels,spectra,envelopes,polarity;};
struct Render {std::vector<float> y;std::array<std::vector<float>,3> taps;double us=0,mean=0;uint32_t maximum=0;};
Render render(const lab::Wave& x,KlonControls c,const std::string& label,Receipts& csv,bool taps=false) {
    Klon m;lab::require(m.prepare(float(x.rate),c),"Klon preparation failed");Render r;r.y.resize(x.samples.size());if(taps)for(auto& a:r.taps)a.resize(x.samples.size());
    std::array<uint64_t,64> bins{};double clipMin=99,clipMax=-99,driveMin=99,driveMax=-99,sumMin=99,sumMax=-99,toneMin=99,toneMax=-99,refMin=99,refMax=-99,kcl=0,pathError=0,d2=0,d3=0;
    for(std::size_t n=0;n<r.y.size();++n){r.y[n]=m.process(x.samples[n]);const auto& v=m.nodes();const auto& p=m.feedbackContributions();
        if(taps)for(unsigned k=0;k<3;++k)r.taps[k][n]=float(p[k]/8);
        clipMin=std::min(clipMin,v[Klon::Clip]);clipMax=std::max(clipMax,v[Klon::Clip]);driveMin=std::min(driveMin,v[Klon::DriveOut]);driveMax=std::max(driveMax,v[Klon::DriveOut]);sumMin=std::min(sumMin,v[Klon::SumOut]);sumMax=std::max(sumMax,v[Klon::SumOut]);toneMin=std::min(toneMin,v[Klon::ToneOut]);toneMax=std::max(toneMax,v[Klon::ToneOut]);refMin=std::min(refMin,v[Klon::Vref]);refMax=std::max(refMax,v[Klon::Vref]);
        kcl=std::max(kcl,std::abs(m.mixResidualAmps()));pathError=std::max(pathError,std::abs(p[0]+p[1]+p[2]-v[Klon::SumOut]+v[Klon::SumMinus]));d2=std::max(d2,std::abs(m.diodeAmps(0)));d3=std::max(d3,std::abs(m.diodeAmps(1)));
        const auto bucket=unsigned(std::clamp(int((v[Klon::Clip]+.5)*64),0,63));++bins[bucket];
        if(n%512==0){csv.trace<<label<<','<<c.faultyContact<<','<<x.rate<<','<<n<<','<<(double(n)+11./12)/x.rate<<','<<x.samples[n]<<','<<r.y[n];for(double a:v)csv.trace<<','<<a;for(unsigned k=0;k<4;++k)csv.trace<<','<<m.amplifierVolts(k);for(unsigned k=0;k<Klon::kCaps;++k)csv.trace<<','<<m.capacitorVolts(k);for(double a:m.mixCurrents())csv.trace<<','<<a;for(double a:p)csv.trace<<','<<a;csv.trace<<','<<m.diodeAmps(0)<<','<<m.diodeAmps(1)<<','<<m.mixResidualAmps()<<','<<m.residualVolts()<<','<<m.failures()<<'\n';}
        const double time=double(n)/x.rate;
        if(label=="balanced"&&time>=.06&&time<.068){csv.polarity<<c.faultyContact<<','<<time<<','<<(double(n)+11./12)/x.rate<<','<<x.samples[n]<<','<<r.y[n]<<','<<v[Klon::Clip]<<','<<v[Klon::SumMinus]<<','<<v[Klon::SumOut]<<','<<v[Klon::ToneOut];for(double a:m.mixCurrents())csv.polarity<<','<<a;for(double a:p)csv.polarity<<','<<a;csv.polarity<<'\n';}
    }
    lab::require(!m.failures()&&!m.invalidInputs()&&!m.inputClamps()&&!m.outputClamps(),"Refuse numerical containment as faulty-contact sound");lab::require(kcl<1e-10&&pathError<1e-5,"Loaded path decomposition failed");
    r.mean=double(m.evaluations())/m.solves();r.maximum=m.maximumEvaluations();
    csv.stages<<label<<','<<c.faultyContact<<','<<clipMin<<','<<clipMax<<','<<driveMin<<','<<driveMax<<','<<sumMin<<','<<sumMax<<','<<toneMin<<','<<toneMax<<','<<refMin<<','<<refMax<<','<<d2<<','<<d3<<','<<m.railSamples()<<','<<m.slewSamples()<<','<<kcl<<','<<pathError<<'\n';
    for(unsigned b=0;b<bins.size();++b)csv.hist<<label<<','<<c.faultyContact<<','<<(-.5+double(b)/64)<<','<<(-.5+double(b+1)/64)<<','<<bins[b]<<'\n';
    m.reset();std::vector<float> repeat(r.y.size());const auto start=std::chrono::steady_clock::now();for(std::size_t n=0;n<repeat.size();++n)repeat[n]=m.process(x.samples[n]);r.us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();lab::require(repeat==r.y&&!m.failures(),"Reset/timed full-process audio differs");return r;
}
void spectrum(std::ofstream& f,const std::string& label,unsigned rate,const std::vector<float>& y) {
    constexpr unsigned size=8192;lab::require(y.size()>=size,"Spectrum too short");const auto start=std::min<std::size_t>(rate,y.size()-size);std::vector<std::complex<double>> a(size);double sum=0;
    for(unsigned i=0;i<size;++i){const double w=.5-.5*std::cos(2*pi*i/(size-1));a[i]=y[start+i]*w;sum+=w;}fft(a);
    for(unsigned i=0;i<=size/2;++i)f<<label<<','<<double(rate)*i/size<<','<<std::abs(a[i])*(i==0||i==size/2?1:2)/sum<<','<<rate<<','<<start<<','<<size<<'\n';
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out) {
    auto original=input.empty()?soundforge::syntheticPlucks():lab::read(input);lab::require(original.samples.size()>=original.rate&&original.samples.size()<=28*original.rate,"Input must be1–28s before2s tail");const auto raw=soundforge::stats(original.samples);lab::require(raw.peak>0,"Cannot calibrate silence");
    std::vector<std::string> names;
    const std::array<std::string,3> labels={"low","medium","high"};
    for(unsigned g=0;g<3;++g)for(unsigned k=0;k<6;++k)names.push_back(std::to_string(g)+std::to_string(k)+"_"+labels[g]+"_"+(k%3==0?"dry":k%3==1?"healthy":"contact100k")+(k<3?"_raw.wav":"_matched.wav"));
    for(const char* kind:{"attacks","long_note","impulse_release","chord"})for(const char* v:{"input_raw","healthy_raw","contact100k_raw","healthy_common","contact100k_common"})names.push_back(std::string(kind)+"_"+v+".wav");
    for(const char* level:{"low_guitar","high_guitar"})for(const char* v:{"healthy","contact100k"})names.push_back(std::string(level)+"_"+v+"_raw.wav");
    for(const char* v:{"healthy","contact100k"})for(const char* tap:{"drive","clean1","clean2"})names.push_back(std::string("analysis/medium_")+v+"_"+tap+".wav");
    auto paths=names;for(const char* p:{"renders.csv","state.csv","stage_stats.csv","histograms.csv","spectrum.csv","envelopes.csv","polarity.csv","settings.json","LISTEN.txt"})paths.push_back(p);
    if(!input.empty())for(const auto& name:paths){const auto p=out/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output aliases input source");}
    std::filesystem::create_directories(out/"analysis");
    Receipts csv;csv.trace.open(out/"state.csv");csv.stages.open(out/"stage_stats.csv");csv.hist.open(out/"histograms.csv");csv.levels.open(out/"renders.csv");csv.spectra.open(out/"spectrum.csv");csv.envelopes.open(out/"envelopes.csv");csv.polarity.open(out/"polarity.csv");
    for(auto* p:{&csv.trace,&csv.stages,&csv.hist,&csv.levels,&csv.spectra,&csv.envelopes,&csv.polarity}){lab::require(bool(*p),"Cannot write receipt");*p<<std::setprecision(15);}
    csv.trace<<"condition,fault,rate_hz,frame,last_phase11_s,input_V,filtered_normalized_output,input,buffer,pre,clean_upper,drive_plus,feedback,drive_minus,drive_out,clip,postclip,clean_wiper,ff1,sum_minus,sum_out,tone_minus,tone_out,level_top,bypass_hp,bypass,jack,Vref";for(unsigned k=0;k<4;++k)csv.trace<<",amp"<<k<<"_relative4V5";for(unsigned k=0;k<Klon::kCaps;++k)csv.trace<<",cap"<<k<<"_V";csv.trace<<",drive_A,clean1_A,clean2_A,drive_feedback_V,clean1_feedback_V,clean2_feedback_V,D2_A,D3_A,KCL_A,port_residual_V,solver_failures\n";
    csv.stages<<"condition,fault,clip_min_V,clip_max_V,drive_min_V,drive_max_V,sum_min_V,sum_max_V,tone_min_V,tone_max_V,Vref_min,Vref_max,D2_peak_A,D3_peak_A,physical_rail_samples,physical_slew_samples,KCL_peak_A,path_sum_peak_V\n";
    csv.hist<<"condition,fault,clip_low_V,clip_high_V,count\n";csv.spectra<<"condition,hz,peak_amplitude,rate_hz,start_frame,hann_frames\n";csv.envelopes<<"fault,attack,start_s,end_s,normalized_output_rms\n";
    csv.polarity<<"fault,frame_s,last_phase11_s,input_V,filtered_output,clip_V,sum_minus_V,sum_out_V,tone_out_V,drive_A,clean1_A,clean2_A,drive_feedback_V,clean1_feedback_V,clean2_feedback_V\n";
    csv.levels<<"file,source_kind,rate_hz,frames,gain,treble,level,fault,input_gain_V_per_source_unit,software_V_per_output_unit,offline_gain,raw_rms,raw_peak,final_rms,final_peak,dc,crest_db,clipped_samples,solver_failures,numerical_containment,mean_evaluations,max_evaluations,host_processor_us\n";
    const auto music=[&](double peak){auto x=original;for(auto& a:x.samples)a=float(a*peak/raw.peak);x.samples.resize(x.samples.size()+2*x.rate,0);return x;};
    auto write=[&](const std::string& name,const lab::Wave& x,const std::vector<float>& signal,KlonControls c,const Render* r,const std::string& kind,double inGain,double gain) {
        auto y=signal;for(auto& a:y)a=float(a*gain);const auto a=soundforge::stats(signal),b=soundforge::stats(y);lab::require(b.peak<.999,"Refuse clipped evidence");lab::write(out/name,{x.rate,y});
        csv.levels<<name<<','<<kind<<','<<x.rate<<','<<y.size()<<','<<c.gain<<','<<c.treble<<','<<c.level<<','<<c.faultyContact<<','<<inGain<<",8,"<<gain<<','<<a.rms<<','<<a.peak<<','<<b.rms<<','<<b.peak<<','<<b.mean<<','<<soundforge::db(b.peak/std::max(1e-30,b.rms))<<",0,0,0,"<<(r?r->mean:0)<<','<<(r?r->maximum:0)<<','<<(r?r->us:0)<<'\n';
    };
    const auto x=music(.2);auto dry=x.samples;for(auto& a:dry)a/=8;
    const auto sourceKind=input.empty()?"owned-synthetic":"caller-provided-PCM16";
    for(unsigned g=0;g<3;++g){KlonControls c{g==0?0.f:g==1?.5f:.95f,.5f,g==0?.85f:.5f,false};const auto h=render(x,c,labels[g],csv,g==1);c.faultyContact=true;const auto f=render(x,c,labels[g],csv,g==1);
        const std::vector<std::vector<float>> signals={dry,h.y,f.y};double target=soundforge::rmsTarget(signals,std::pow(10.,-23./20),.89);for(const auto& y:signals)target=std::min(target,16*soundforge::stats(y).rms);
        for(unsigned k=0;k<6;++k){auto setting=c;setting.faultyContact=k%3==2;write(names[g*6+k],x,signals[k%3],setting,k%3==0?nullptr:k%3==1?&h:&f,sourceKind,.2/raw.peak,k<3?1:soundforge::rmsGain(soundforge::stats(signals[k%3]),target));}
        spectrum(csv.spectra,labels[g]+"_dry",x.rate,dry);spectrum(csv.spectra,labels[g]+"_healthy",x.rate,h.y);spectrum(csv.spectra,labels[g]+"_contact100k",x.rate,f.y);
        if(g==1){double peak=0;for(const auto* r:{&h,&f})for(const auto& y:r->taps)peak=std::max(peak,soundforge::stats(y).peak);const double common=std::min(1.,.89/peak);for(unsigned fault=0;fault<2;++fault)for(unsigned k=0;k<3;++k){const auto& r=fault?f:h;auto setting=c;setting.faultyContact=fault!=0;write(names[42+fault*3+k],x,r.taps[k],setting,&r,"analysis_feedback_volts_before_tone",.2/raw.peak,common);}}
    }
    for(unsigned kind=0;kind<4;++kind){lab::Wave a;a.rate=48000;a.samples.resize(kind==0?48000:kind==1?96000:kind==2?28800:144000);for(std::size_t n=0;n<a.samples.size();++n){const double t=double(n)/a.rate;a.samples[n]=kind==0?klonlab::attacks(t):kind==1?klonlab::note(t):kind==2?klonlab::impulse(t):klonlab::chord(t);}
        const auto label=kind==0?"attacks":kind==1?"long_note":kind==2?"impulse_release":"chord";KlonControls c;const auto h=render(a,c,label,csv);c.faultyContact=true;const auto f=render(a,c,label,csv);auto d=a.samples;for(auto& v:d)v/=8;const double common=std::min(8.,.89/std::max(soundforge::stats(h.y).peak,soundforge::stats(f.y).peak));
        c.faultyContact=false;write(names[18+kind*5],a,d,c,nullptr,"owned-controlled",1,1);write(names[19+kind*5],a,h.y,c,&h,"owned-controlled",1,1);write(names[21+kind*5],a,h.y,c,&h,"owned-controlled",1,common);c.faultyContact=true;write(names[20+kind*5],a,f.y,c,&f,"owned-controlled",1,1);write(names[22+kind*5],a,f.y,c,&f,"owned-controlled",1,common);
        if(kind==0)for(unsigned fault=0;fault<2;++fault)for(unsigned k=0;k<3;++k){const double start=.03+.22*k,end=start+.1;double e=0;unsigned count=0;for(unsigned n=unsigned(start*a.rate);n<unsigned(end*a.rate);++n){const double v=(fault?f:h).y[n];e+=v*v;++count;}csv.envelopes<<fault<<','<<k<<','<<start<<','<<end<<','<<std::sqrt(e/count)<<'\n';}
    }
    for(unsigned level=0;level<2;++level){const double peak=level?.4:.04;const auto a=music(peak);for(unsigned fault=0;fault<2;++fault){KlonControls c{.5f,.5f,.5f,fault!=0};const auto r=render(a,c,level?"high_guitar":"low_guitar",csv);write(names[38+2*level+fault],a,r.y,c,&r,sourceKind,peak/raw.peak,1);}}
    lab::Wave balanced;balanced.rate=48000;balanced.samples.resize(5760);for(std::size_t n=0;n<balanced.samples.size();++n)balanced.samples[n]=klonlab::periodic(double(n)/balanced.rate,.2,1000);for(bool fault:{false,true})render(balanced,{.5f,.5f,.5f,fault},"balanced",csv);
    std::ofstream settings(out/"settings.json"),notes(out/"LISTEN.txt");lab::require(bool(settings)&&bool(notes),"Cannot write settings");
    settings<<std::setprecision(15)<<"{\"standalone_only\":true,\"source_kind\":\""<<sourceKind<<"\",\"input_peak_model_V\":0.2,\"source_calibration\":"<<.2/raw.peak<<",\"software_V_per_output_unit\":8,\"contact_part\":\"P1A wiper track to external clean lug\",\"healthy_contact_ohms\":0,\"fault_contact_ohms\":100000,\"unchanged_drive_gang\":\"P1B\",\"gain_settings\":[0,0.5,0.95],\"output_settings\":[0.85,0.5,0.5],\"treble\":0.5,\"main_input_V\":9,\"pump_positive_V\":16.2,\"pump_negative_V\":-8.6,\"Vref\":\"actual27k/27k/47u divider\",\"oversampling\":12,\"FIR_taps\":577,\"nominal_FIR_delay_frames\":48,\"maximum_music_matching_gain\":16,\"state_bytes\":"<<sizeof(Klon)<<"}\n";
    notes<<"SF-011: experimental original-style S698 signal circuit, one P1A wiper contact0->100k ohm. P1B and all other components/controls identical. Not a calibrated vintage Centaur/1N34A/TL072 sample; no founder approval.\n"
         <<"Start14_medium_healthy_matched.wav /15_medium_contact100k_matched.wav; compare11/12 RAW, then low04/05 and high24/25. Whole-file RMS target-23dBFS,peak.89,gain<=16, not LUFS. Dry/input and processed outputs share softwareVolts/8; raw gains1. Matching may choose a lower common target when the16x cap binds. Actual raw/matched levels, gains and crest inrenders.csv.\n"
         <<"The same new guitar excerpt/calibration(.2modelV peak)/2s tail appears in each Gain pair. Low Gain0/Output.85, medium.5/.5, high.95/.5, Treble.5. Four extra low/high guitar RAW files use.04/.4modelV peak at medium controls. Different preset controls/levels are disclosed; do not infer gain-only comparisons across groups.\n"
         <<"analysis/*.wav are diagnostic feedback-voltage contributions BEFORE Tone, at lastphase11, with one disclosed common safety gain; no output-level matching. Drive=C10 current, clean1=R19 current, clean2=C11/R15+R17+C12/R18 current, all into the ACTUAL loaded summing node. Each is passed through -R20/(1+sR20C13); their sum reconstructs SumOut-SumMinus including finite virtual-node error, not the final Tone/Level WAV. They are not independent isolated clean/dirty sources. R16 alone also contains clean C11 feedthrough.\n"
         <<"state/polarity use last internalphase11 time(frame+11./12)/rate. Filtered WAV decimates phase0 with nominal48frame combined FIR delay. The taps are BEFORE Tone/FIR and have different timing/units. KCL/path residuals, all nodes/caps/states/diode currents, histogram, phase/current SPICE and spectra are numerical evidence, not hearing.\n"
         <<"Controlled attacks/long_note/impulse_release/chord and balanced sine are OWNED SYNTHETIC. Arbitrary --input does not imply rights; CI supplies the pinned new Guitar-TECHS take03 original/license/20s conversion. No GuitarJam050 or take06/take11 in this package.\n"
         <<"Original ICL7660S inverter+doubler supply domains use FIXED nominal-8.6/+16.2V and9V, plus actual passive Vref; no physical pump ripple/sag/startup/current calibration. Generic Ge Shockley and reduced legacy TL072 pole/slew/rails omit intrinsic charge, offset/noise/common-mode reversal/current limiting/detailed recovery. No Engine/patch/firmware/hardware admission.\n"
         <<"Physical rail/slew counters are separate from numerical failures/clamps; ordinary renders require all numerical containmentZERO. Full-process host timing is a separate reset/repeat pass including native diagnostics/FIR, excluding prepare/trace/IO/analysis. No measured Seed3 cycles/deadline/physical audio or pleasantness claim.\n";
    for(auto* p:{&csv.trace,&csv.stages,&csv.hist,&csv.levels,&csv.spectra,&csv.envelopes,&csv.polarity,&settings,&notes}){p->flush();lab::require(bool(*p),"Cannot finish evidence");}
    std::cout<<"Created48 native Klon WAVs:18 musical,20 controlled,4 guitar-level probes,6 analysis-only loaded path contributions in "<<out<<'\n';
}
}
int main(int argc,char** argv){try{std::filesystem::path input,out;bool si=false,so=false;for(int i=1;i<argc;++i){const std::string k=argv[i];lab::require((k=="--input"||k=="--output")&&i+1<argc,"Use --output DIR [--input monoPCM16.wav]");const std::string v=argv[++i];lab::require(!v.empty(),"Empty path");bool& seen=k=="--input"?si:so;lab::require(!seen,"Duplicate argument");seen=true;(k=="--input"?input:out)=v;}lab::require(so,"Explicit output required");audition(input,out);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
