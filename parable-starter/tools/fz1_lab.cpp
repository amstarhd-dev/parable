// Actual standalone native FZ1. Host-only IO, analysis and measured complete render.
#include "parable_fz1.h"
#include "parable_germanium.h"
#include "parable_muff.h"
#include "parable_rat.h"
#include "parable_cmos.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <set>
using parable::dsp::Fz1;using parable::dsp::Fz1Config;
namespace {
constexpr double pi=3.14159265358979323846;
struct Receipts {std::ofstream levels,state,cost;};
std::vector<float> render(const lab::Wave& x,float attack,bool fault,const std::string& label,Receipts& f) {
 Fz1Config c;c.attack=attack;c.openBiasFeed=fault;Fz1 m;lab::require(m.prepare(float(x.rate),c),"FZ1 prepare failed");std::vector<float> y(x.samples.size()),repeat(y.size());
 for(std::size_t i=0;i<y.size();++i){y[i]=m.process(x.samples[i]);if(i%512==0){const auto n=m.nodes();f.state<<label<<','<<fault<<','<<i<<','<<(i+.75)/x.rate<<','<<x.samples[i]<<','<<m.jackVolts();for(double v:n)f.state<<','<<v;for(unsigned q=0;q<3;++q)f.state<<','<<m.collectorAmps(q);for(unsigned k=0;k<4;++k)f.state<<','<<m.capacitorVolts(k);f.state<<','<<m.residualVolts()<<'\n';}}
 lab::require(!m.failures()&&!m.inputClamps()&&!m.invalidInputs()&&!m.outputClamps(),"Containment is not physical fault sound");
 m.reset();const auto cpu=std::clock();const auto start=std::chrono::steady_clock::now();for(std::size_t i=0;i<y.size();++i)repeat[i]=m.process(x.samples[i]);const double wall=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/y.size(),cpuUs=1e6*double(std::clock()-cpu)/CLOCKS_PER_SEC/y.size();
 lab::require(y==repeat&&!m.failures()&&!m.inputClamps()&&!m.outputClamps(),"Timed full processor reset repeat failed");f.cost<<label<<','<<fault<<','<<x.rate<<','<<y.size()<<','<<wall<<','<<cpuUs<<','<<sizeof(m)<<','<<m.maximumEvaluations()<<','<<double(m.evaluations())/m.solves()<<",0,0,0\n";return y;
}
void save(const std::filesystem::path& out,const std::string& name,unsigned rate,const std::vector<float>& raw,double gain,double inputGain,float attack,const std::string& model,Receipts& f) {
 lab::require(gain>0&&gain<=16.000001,"Offline gain exceeds16");const auto before=soundforge::stats(raw);lab::Wave w{rate,raw};for(auto& a:w.samples)a=float(a*gain);const auto after=soundforge::stats(w.samples);lab::require(after.peak<.999,"WAV clips");lab::write(out/name,w);
 f.levels<<name<<','<<model<<','<<rate<<','<<raw.size()<<','<<inputGain<<','<<attack<<','<<before.rms<<','<<before.peak<<','<<before.mean<<','<<gain<<','<<after.rms<<','<<after.peak<<','<<after.mean<<'\n';
}
void group(const lab::Wave& x,const std::string& label,float attack,double calibration,const std::filesystem::path& out,Receipts& f,bool matched) {
 std::vector<std::vector<float>> s(3);s[0]=x.samples;for(auto& v:s[0])v/=4;s[1]=render(x,attack,false,label,f);s[2]=render(x,attack,true,label,f);
 double common=16,target=soundforge::rmsTarget(s,.1,.9);for(const auto& v:s){const auto q=soundforge::stats(v);if(q.rms>0){common=std::min(common,.1/q.rms);target=std::min(target,16*q.rms);}if(q.peak>0)common=std::min(common,.9/q.peak);}
 for(unsigned k=0;k<3;++k){const std::string name=label+"_"+(k==0?"dry":k==1?"healthy":"feed_open"),model=k==0?"dry":k==1?"FZ1-healthy":"FZ1-feed-open";save(out,name+"_raw.wav",x.rate,s[k],1,calibration,attack,model,f);save(out,name+"_common.wav",x.rate,s[k],common,calibration,attack,model,f);if(matched)save(out,name+"_rms_matched.wav",x.rate,s[k],soundforge::rmsGain(soundforge::stats(s[k]),target),calibration,attack,model,f);}
}
void comparison(const lab::Wave& x,double calibration,const std::filesystem::path& out,Receipts& f) {
 std::vector<std::vector<float>> s(6);s[0]=x.samples;for(auto& v:s[0])v/=4;s[1]=render(x,.5f,false,"comparison",f);for(unsigned k=2;k<6;++k)s[k].resize(x.samples.size());
 parable::dsp::Germanium ge;parable::dsp::Muff muff;parable::dsp::Rat rat;parable::dsp::Cmos cmos;lab::require(ge.configure(float(x.rate),{.5f,.5f})&&muff.prepare(float(x.rate),{false,.5f,.5f,.5f})&&rat.configure(float(x.rate),{.5f,.5f,.5f})&&cmos.configure(float(x.rate),{.5f,.5f}),"Existing comparison configure failed");
 for(std::size_t i=0;i<x.samples.size();++i){s[2][i]=ge.process(x.samples[i])/4;s[3][i]=muff.process(x.samples[i]);s[4][i]=rat.process(x.samples[i]);s[5][i]=cmos.process(x.samples[i]);}
 lab::require(!ge.failedSolves()&&!ge.containedOutputs()&&!muff.failures()&&!muff.inputClamps()&&!muff.outputClamps()&&!cmos.failedSolves(),"Existing comparator containment");
 double target=soundforge::rmsTarget(s,.1,.9);for(const auto& v:s)target=std::min(target,16*soundforge::stats(v).rms);const char* names[]={"dry","FZ1","germanium_2T","Muff_4T","RAT","CMOS"};
 for(unsigned k=0;k<6;++k){save(out,std::string("compare_")+names[k]+"_raw.wav",x.rate,s[k],1,calibration,.5f,names[k],f);save(out,std::string("compare_")+names[k]+"_rms_matched.wav",x.rate,s[k],soundforge::rmsGain(soundforge::stats(s[k]),target),calibration,.5f,names[k],f);}
}
std::vector<std::string> outputNames(){std::vector<std::string> names;for(const char* group:{"attack_low","attack_mid","attack_high","input_low","input_mid","input_high","pluck_decay","held_note","volume_rollback","hard_attack","chord","two_tone"})for(const char* state:{"dry","healthy","feed_open"})for(const char* level:{"raw","common","rms_matched"})names.push_back(std::string(group)+"_"+state+"_"+level+".wav");for(const char* n:{"dry","FZ1","germanium_2T","Muff_4T","RAT","CMOS"})for(const char* l:{"raw","rms_matched"})names.push_back(std::string("compare_")+n+"_"+l+".wav");for(const char* n:{"renders.csv","state.csv","performance.csv","settings.json","LISTEN.txt"})names.push_back(n);return names;}
void audition(const std::filesystem::path& input,const std::filesystem::path& out,bool compare){
 auto original=input.empty()?soundforge::syntheticPlucks():lab::read(input);lab::require(original.rate==44100||original.rate==48000||original.rate==96000,"Use44.1/48/96k");lab::require(original.samples.size()>=original.rate&&original.samples.size()<=26*original.rate,"Use1–26s input");const auto stats=soundforge::stats(original.samples);lab::require(stats.peak>0,"Cannot calibrate silence");
 // Reject source aliases before opening ANY output. Directory symlinks/hard links count.
 if(!input.empty())for(const auto& n:outputNames()){const auto p=out/n;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output aliases input");}
 std::filesystem::create_directories(out);Receipts f;f.levels.open(out/"renders.csv");f.state.open(out/"state.csv");f.cost.open(out/"performance.csv");for(auto* s:{&f.levels,&f.state,&f.cost}){lab::require(bool(*s),"Cannot create receipt");*s<<std::setprecision(17);}
 f.levels<<"file,model,rate_hz,frames,input_gain_V_per_source_unit,attack_fraction,raw_rms,raw_peak,raw_mean,offline_gain,final_rms,final_peak,final_mean\n";f.state<<"group,fault,frame,last_internal_sample_s,input_V,internal_jack_V,b1_V,e1_V,b2_V,c2_V,b3_V,c3_V,q1_ic_A,q2_ic_A,q3_ic_A,c1_V,c2_V,c3_V,c4_V,residual_V\n";f.cost<<"group,fault,rate_hz,frames,wall_us_per_frame,CPU_us_per_frame,state_bytes,max_evaluations,mean_evaluations,failures,input_clamps,output_clamps\n";
 const double calibration=.2/stats.peak;
 for(unsigned g=0;g<6;++g){const float attack=g==0?.1f:g==2?1.f:.5f;const double level=g==3?.02:g==4?.1:.2;auto x=original;for(auto& v:x.samples)v=float(v*level/stats.peak);x.samples.resize(x.samples.size()+2*x.rate);const char* names[]={"attack_low","attack_mid","attack_high","input_low","input_mid","input_high"};group(x,names[g],attack,level/stats.peak,out,f,true);}
 for(const std::string kind:{"pluck_decay","held_note","volume_rollback","hard_attack","chord","two_tone"}){lab::Wave x{48000,{}};const unsigned frames=kind=="held_note"?4*48000:2*48000;x.samples.reserve(frames+2*48000);for(unsigned i=0;i<frames;++i){const double t=double(i)/48000;double y=0;if(kind=="pluck_decay"){const double a=t-.01;y=a<0?0:.2*(1-std::exp(-3000*a))*std::exp(-a/.12)*std::sin(2*pi*110*a);}else if(kind=="held_note"){const double a=std::min(1.,t*1000),decay=t<2?1:std::exp(-(t-2)/.2);y=.2*a*decay*std::sin(2*pi*196*t);}else if(kind=="volume_rollback"){const double level=t<.5?.2:t<1?.1:t<1.5?.02:.2;y=level*std::sin(2*pi*196*t);}else if(kind=="hard_attack"){const double a=std::fmod(t,.5);y=.2*(1-std::exp(-10000*a))*std::exp(-a/.08)*(std::sin(2*pi*110*t)+.2*std::sin(2*pi*330*t))/1.2;}else if(kind=="chord")for(double hz:{110.,146.832,164.814})y+=.2/3*std::sin(2*pi*hz*t);else y=.1*(std::sin(2*pi*1760*t)+std::sin(2*pi*3520*t));x.samples.push_back(float(y));}x.samples.resize(x.samples.size()+2*x.rate);group(x,kind,.5f,1,out,f,false);}
 if(compare){auto x=original;for(auto& v:x.samples)v=float(v*calibration);x.samples.resize(x.samples.size()+2*x.rate);comparison(x,calibration,out,f);}
 for(auto* s:{&f.levels,&f.state,&f.cost}){s->flush();lab::require(bool(*s),"Receipt write failed");}
 std::ofstream settings(out/"settings.json");settings<<"{\"experimental\":true,\"model\":\"original-FZ1-style measured-AC128-template\",\"supply_V\":-3,\"source_ohms\":1000,\"amp_load_ohms\":1000000,\"volume\":0.5,\"healthy_feed_ohms\":470000,\"fault\":\"only Q2 feed physically open\",\"Attack\":[0.1,0.5,1],\"input_peak_V\":[0.02,0.1,0.2],\"normal_factor\":8,\"FIR_taps\":385,\"output_divisor\":4,\"tail_s\":2,\"offline_gain_ceiling\":16,\"state_bytes\":"<<sizeof(Fz1)<<",\"hardware_timing_measured\":false,\"human_audition_approved\":false}\n";
 std::ofstream listen(out/"LISTEN.txt");listen<<"SF015 original-FZ1-style. Start attack_mid_dry_common / healthy_common / feed_open_common, then RMS matched. Attack0.1/0.5/1 at identical0.2V peak; input0.02/0.1/0.2V at Attack0.5. Same take03 as Centaur/tape/Aquarius,20s plus2s natural tail. Raw is native jack/4 and dry/4, no offline compensation. Common gain is identical within each triple; RMS matched has disclosed individual gains, not LUFS. Complete physical bias/capacitor/current traces and levels are CSV. Exactly one fault: open Q2 470k feed; all devices/rail/controls identical. Physical3PNP clipping/memory, not synthetic gating: this sample has a finite smooth low-level tail; no abrupt hard-gate claim or founder approval. Owned pluck/held-note/volume-rollback/hard-attack/chord/two-tone files are synthetic, not recorded guitar. Optional compare_* uses SAME take, input0.2V,2s tail, all prior controls0.5, FZ1Attack/Volume0.5. Raw divisors: dry/FZ1/2T-Ge4, Muff9,RAT/CMOS4; unequal circuits/gain. RMS matches support structural comparison, not superiority. No physical hardware run or measured Seed3 cycles.\n";
 settings.flush();listen.flush();lab::require(bool(settings)&&bool(listen),"Settings write failed");
}
}
int main(int argc,char** argv)try{std::filesystem::path in,out;std::set<std::string> seen;bool compare=false;for(int i=1;i<argc;++i){const std::string key=argv[i];lab::require(seen.insert(key).second,"Duplicate option");if(key=="--compare-existing"){compare=true;continue;}lab::require((key=="--input"||key=="--output")&&i+1<argc,"Unknown/missing option");const std::string v=argv[++i];lab::require(!v.empty(),"Empty option");if(key=="--input")in=v;else out=v;}lab::require(!out.empty(),"Require --output, optional --input monoPCM16.wav and --compare-existing");audition(in,out,compare);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
