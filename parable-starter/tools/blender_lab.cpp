// Same native circuit as tests; host-only files, PCM, statistics and timing.
#include "parable_blender.h"
#include "parable_rat.h"
#include "parable_muff.h"
#include "parable_cmos.h"
#include "parable_wavefolder.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
using parable::dsp::Blender;
using parable::dsp::BlenderConfig;
namespace {
constexpr double pi=3.14159265358979323846;
struct Files {std::ofstream levels,state,cost;};
struct Setting {const char* name;float sustain,blend;bool boost;};
constexpr std::array<Setting,5> settings={{{"sustain_low_b100_boost0",.15f,1,false},{"sustain_mid_b100_boost0",.5f,1,false},{"sustain_high_b100_boost0",1,1,false},{"sustain_mid_b10_boost0",.5f,.1f,false},{"sustain_mid_b50_boost1",.5f,.5f,true}}};
std::vector<float> render(const lab::Wave& x,Setting s,bool fault,const std::string& label,Files& files){
    Blender m;BlenderConfig c;c.sustain=s.sustain;c.blend=s.blend;c.boost=s.boost;c.d1Open=fault;lab::require(m.prepare(float(x.rate),c),"Native Blender preparation failed");
    std::vector<float> y(x.samples.size()),repeat(y.size());
    for(std::size_t n=0;n<y.size();++n){y[n]=m.process(x.samples[n]);if(n%128==0){files.state<<label<<','<<fault<<','<<n<<','<<(n+double(m.factor()-1)/m.factor())/x.rate<<','<<x.samples[n]<<','<<m.jackVolts();for(double v:m.nodes())files.state<<','<<v;for(unsigned d=0;d<4;++d)files.state<<','<<m.diodeBranchAmps(d);files.state<<','<<m.residualVolts()<<'\n';}}
    lab::require(!m.failures()&&!m.inputClamps()&&!m.outputClamps()&&!m.invalidInputs(),"Numerical containment cannot be fault sound");
    m.reset();const auto cpu=std::clock();const auto start=std::chrono::steady_clock::now();for(std::size_t n=0;n<y.size();++n)repeat[n]=m.process(x.samples[n]);
    const double cpuUs=1e6*double(std::clock()-cpu)/CLOCKS_PER_SEC,us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(y==repeat&&!m.failures()&&!m.inputClamps()&&!m.outputClamps()&&!m.invalidInputs(),"Deterministic second native render failed");
    files.cost<<label<<','<<fault<<','<<x.rate<<','<<y.size()<<','<<m.factor()<<','<<cpuUs/y.size()<<','<<us/y.size()<<','<<double(m.evaluations())/m.solves()<<','<<m.maximumEvaluations()<<','<<sizeof(m)<<",0,0,0\n";std::cout<<label<<' '<<(fault?"D1-open":"healthy")<<" rendered and repeated\n"<<std::flush;return y;
}
void save(const std::filesystem::path& out,const std::string& name,const char* model,unsigned rate,const std::vector<float>& raw,double gain,double calibration,Setting s,bool fault,Files& f){
    lab::require(gain>0&&gain<=16.00001,"Offline gain must be0..16");const auto before=soundforge::stats(raw);lab::Wave w{rate,raw};for(auto& v:w.samples)v=float(v*gain);const auto after=soundforge::stats(w.samples);lab::require(after.peak<.999,"Refuse audition clipping");lab::write(out/name,w);const auto delivered=lab::read(out/name);const auto pcm=soundforge::stats(delivered.samples);
    f.levels<<name<<','<<model<<','<<rate<<','<<raw.size()<<','<<calibration<<','<<s.sustain<<",0.5,"<<s.blend<<",0.5,"<<s.boost<<','<<fault<<','<<before.rms<<','<<before.peak<<','<<before.mean<<','<<gain<<','<<after.rms<<','<<after.peak<<','<<after.mean<<','<<pcm.rms<<','<<pcm.peak<<','<<pcm.mean<<",0,0,0,0\n";
}
double target(const std::vector<std::vector<float>>& signals){double t=soundforge::rmsTarget(signals,.1,.9);for(const auto& x:signals){const double rms=soundforge::stats(x).rms;if(rms>0)t=std::min(t,16*rms);}return t;}
void group(const lab::Wave& x,const std::string& label,Setting setting,double calibration,const std::filesystem::path& out,Files& f,bool matched){
    std::vector<std::vector<float>> signals(3);signals[0]=x.samples;for(auto& v:signals[0])v/=8;signals[1]=render(x,setting,false,label,f);signals[2]=render(x,setting,true,label,f);const char* labels[]={"external_dry","healthy","D1_open"};
    double common=16;for(const auto& x:signals){const auto s=soundforge::stats(x);if(s.rms>0)common=std::min(common,.1/s.rms);if(s.peak>0)common=std::min(common,.9/s.peak);}const double t=target(signals);
    for(unsigned i=0;i<3;++i){const auto prefix=label+"_"+labels[i];save(out,prefix+"_raw.wav",i?"Blender":"external_DI",x.rate,signals[i],1,calibration,setting,i==2,f);save(out,prefix+"_common.wav",i?"Blender":"external_DI",x.rate,signals[i],common,calibration,setting,i==2,f);if(matched)save(out,prefix+"_rms_matched.wav",i?"Blender":"external_DI",x.rate,signals[i],soundforge::rmsGain(soundforge::stats(signals[i]),t),calibration,setting,i==2,f);}
}
void comparison(const lab::Wave& x,double calibration,const std::filesystem::path& out,Files& f){
    Setting s{"comparison",.5f,.5f,false};std::vector<std::vector<float>> v(6);v[0]=x.samples;for(auto& a:v[0])a/=8;v[1]=render(x,s,false,"comparison",f);for(unsigned i=2;i<6;++i)v[i].resize(x.samples.size());
    parable::dsp::Rat rat;parable::dsp::Muff muff;parable::dsp::Cmos cmos;parable::dsp::Wavefolder folder;
    lab::require(rat.configure(float(x.rate),{.5f,.5f,.5f})&&muff.prepare(float(x.rate),{false,.5f,.5f,.5f})&&cmos.configure(float(x.rate),{.5f,.5f})&&folder.prepare(float(x.rate),{3,false}),"Main comparison prepare failed");
    for(std::size_t n=0;n<x.samples.size();++n){v[2][n]=rat.process(x.samples[n]);v[3][n]=muff.process(x.samples[n]);v[4][n]=cmos.process(x.samples[n]);v[5][n]=folder.process(x.samples[n]);}
    lab::require(!muff.failures()&&!muff.outputClamps()&&!cmos.failedSolves()&&!folder.failures()&&!folder.tableClamps()&&!folder.outputClamps(),"Main comparator containment");const double t=target(v);const char* names[]={"external_DI","Blender","RAT","Muff","CMOS","Aquarius"};
    for(unsigned i=0;i<6;++i)for(bool match:{false,true})save(out,std::string("compare_")+names[i]+(match?"_rms_matched.wav":"_raw.wav"),names[i],x.rate,v[i],match?soundforge::rmsGain(soundforge::stats(v[i]),t):1,calibration,s,false,f);
}
std::vector<std::string> paths(){
    std::vector<std::string> names;
    for(const auto s:settings)for(const char* signal:{"external_dry","healthy","D1_open"})for(const char* mode:{"raw","common","rms_matched"})names.push_back(std::string("input_")+s.name+"_"+signal+"_"+mode+".wav");
    for(const char* kind:{"low","high","pluck","chord"})for(const char* signal:{"external_dry","healthy","D1_open"})for(const char* mode:{"raw","common"})names.push_back(std::string("synthetic_")+kind+"_"+signal+"_"+mode+".wav");
    for(const char* model:{"external_DI","Blender","RAT","Muff","CMOS","Aquarius"})for(const char* mode:{"raw","rms_matched"})names.push_back(std::string("compare_")+model+"_"+mode+".wav");
    for(const char* name:{"renders.csv","state.csv","performance.csv","settings.json","LISTEN.txt"})names.push_back(name);
    return names;
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out,bool compare,bool quick){
    auto source=input.empty()?soundforge::syntheticPlucks():lab::read(input);lab::require(source.rate==44100||source.rate==48000||source.rate==96000,"Use44.1/48/96kHz");lab::require(source.samples.size()>=source.rate/100&&source.samples.size()<=28*source.rate,"Input duration .01..28 seconds");
    if(!input.empty())for(const auto& name:paths()){const auto path=out/name;lab::require(std::filesystem::weakly_canonical(path)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(path)||!std::filesystem::equivalent(path,input)),"Output aliases input");}
    if(quick&&input.empty()){source.samples.resize(source.rate/10);for(unsigned n=0;n<source.samples.size();++n)source.samples[n]=float(.05*std::sin(2*pi*196*n/source.rate));}
    if(quick&&source.samples.size()>source.rate/10)source.samples.resize(source.rate/10);
    const double peak=soundforge::stats(source.samples).peak;lab::require(peak>0,"Cannot calibrate silence");const double calibration=.2/peak;for(auto& v:source.samples)v=float(v*calibration);source.samples.resize(source.samples.size()+unsigned(source.rate*(quick?.05:2)));
    std::filesystem::create_directories(out);Files f;f.levels.open(out/"renders.csv");f.state.open(out/"state.csv");f.cost.open(out/"performance.csv");for(auto* stream:{&f.levels,&f.state,&f.cost}){lab::require(bool(*stream),"Cannot open receipt");*stream<<std::setprecision(16);}
    f.levels<<"file,model,rate_hz,frames,input_gain_V_per_source_unit,sustain,tone,blend,level,boost,D1_open,raw_rms,raw_peak,raw_DC,offline_gain,final_float_rms,final_float_peak,final_float_DC,PCM_rms,PCM_peak,PCM_DC,numerical_failures,input_clamps,output_clamps,PCM_clips\n";
    f.state<<"group,D1_open,frame,last_internal_phase_s,input_host_V,jack_before_FIR_V";for(unsigned n=0;n<Blender::kNodes;++n)f.state<<",node"<<n<<"_V";for(unsigned d=1;d<=4;++d)f.state<<",D"<<d<<"_branch_A";f.state<<",residual_V\n";
    f.cost<<"group,D1_open,rate_hz,frames,factor,CPU_us_per_frame,wall_us_per_frame,mean_evaluations_per_internal,max_evaluations,state_bytes,failures,input_clamps,output_clamps\n";
    if(quick)for(auto setting:settings)group(source,"input_"+std::string(setting.name),setting,calibration,out,f,true);
    else{
        for(auto setting:settings)group(source,"input_"+std::string(setting.name),setting,calibration,out,f,true);
        for(const std::string kind:{"low","high","pluck","chord"}){lab::Wave x{48000,{}};for(unsigned n=0;n<48000;++n){const double t=double(n)/x.rate;double y=.05*std::sin(2*pi*(kind=="high"?880:110)*t)*(1-std::exp(-t/.002));if(kind=="pluck")y*=4*std::exp(-t/.12);if(kind=="chord")y=.2/3*(std::sin(2*pi*110*t)+std::sin(2*pi*138.59*t)+std::sin(2*pi*164.81*t))*(1-std::exp(-t/.002));x.samples.push_back(float(y));}x.samples.resize(57600);group(x,"synthetic_"+kind,settings[1],1,out,f,false);}
        if(compare)comparison(source,calibration,out,f);
    }
    for(auto* stream:{&f.levels,&f.state,&f.cost}){stream->flush();lab::require(bool(*stream),"Receipt write failed");}
    std::ofstream settingsFile(out/"settings.json");settingsFile<<"{\"experimental\":true,\"quick_plumbing_only\":"<<(quick?"true":"false")<<",\"supplied_PCM16\":"<<(!input.empty()?"true":"false")<<",\"input_calibration\":"<<std::setprecision(16)<<calibration<<",\"output_divisor\":8,\"input_peak_V\":0.2,\"supply_V\":9,\"temperature_C\":26.85,\"beta\":350,\"source_ohms\":1000,\"load_ohms\":1000000,\"tone\":0.5,\"level\":0.5,\"only_fault\":\"D1_open\",\"normal_factor\":8,\"FIR_taps\":385,\"integration\":\"BDF2\",\"state_bytes\":"<<sizeof(Blender)<<",\"deterministic_second_pass\":true,\"founder_approved\":false}\n";
    std::ofstream listen(out/"LISTEN.txt");listen<<"SF016 documented original-style Fender Blender, not an authenticated vintage specimen. Begin input_sustain_mid_b100_boost0 external_dry/healthy/D1_open common, then RMS-matched. Only the collector-side rectifier D1 opens; raw/common preserve genuine level differences. Input_ uses supplied mono PCM16 or explicitly owned synthetic fallback; package provenance identifies actual take03. All controls are electrical fractions, not mechanical taper. Blend0 is an internal preamp tap, not external DI. Tone Boost rewires the actual network. Raw means native jackVolts/8 with original analog coupling and stated FIR; raw component DC/currents remain in state.csv. Individual RMS matching is not LUFS/subjective approval. Synthetic low/high/pluck/chord are owned, separate from guitar. Main RAT/Muff/CMOS/Aquarius comparisons use the SAME calibrated phrase; topologies/gains/divisors differ, not a superiority test or octave comparator. No credible licensed exact octave comparator was found. All native renders are repeated exactly and reject numerical containment. Quick mode is short plumbing evidence only. Scientific acceptance, physical pedal fidelity, founder listening and Seed3 real-time admission remain separate gates.\n";
    settingsFile.flush();listen.flush();lab::require(bool(settingsFile)&&bool(listen),"Settings write failed");
}
}
int main(int argc,char** argv)try{
    std::filesystem::path input,out;bool in=false,haveOut=false,compare=false,quick=false;
    for(int i=1;i<argc;++i){const std::string a=argv[i];if(a=="--compare-existing"&&!compare){compare=true;continue;}if(a=="--quick"&&!quick){quick=true;continue;}lab::require(i+1<argc,"Missing value");const std::string value=argv[++i];lab::require(!value.empty(),"Empty option value");if(a=="--input"&&!in){input=value;in=true;}else if(a=="--output"&&!haveOut){out=value;haveOut=true;}else throw std::runtime_error("Unknown/duplicate option");}
    lab::require(haveOut,"Require --output directory; optional --input monoPCM16.wav --compare-existing --quick");lab::require(!(compare&&quick),"Comparison requires complete audition mode");audition(input,out,compare,quick);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
