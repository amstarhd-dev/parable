// Same native C++ as tests. Host-only files/analysis/benchmark, never audio code.
#include "parable_wavefolder.h"
#include "wavefolder_cell.h"
#include "wavefolder_stimulus.h"
#include "parable_rat.h"
#include "parable_muff.h"
#include "parable_cmos.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <ctime>
#include <iomanip>
#include <iostream>
using parable::dsp::Wavefolder;
namespace cell=parable::dsp::wavefolder_detail;
namespace {
// Main's owned laboratory FFT, not new shared infrastructure.
void fft(std::vector<std::complex<double>>& a) {
 const auto n=a.size();for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
 for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*foldlab::pi/length);for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
struct Files {std::ofstream levels,state,cost,spectra;};
std::vector<float> render(const lab::Wave& x,float drive,bool fault,const std::string& label,Files& f) {
 Wavefolder m;lab::require(m.prepare(float(x.rate),{drive,fault}),"Wavefolder prepare failed");std::vector<float> y(x.samples.size()),repeat(y.size());
 for(std::size_t n=0;n<y.size();++n){y[n]=m.process(x.samples[n]);if(n%128==0){const auto& p=m.probe();cell::Vec nodes{};const auto observed=cell::observe(p.cellInputVolts,p.instantCellVolts,nodes,m.upperFeedOhms());lab::require(observed.valid&&cell::norm(observed.residual)<2e-7,"Interpolated cell KCL failed");const auto& j=observed.junction;
   f.state<<label<<','<<fault<<','<<x.rate<<','<<n<<','<<(n+.75)/x.rate<<','<<x.samples[n]<<','<<p.cellInputVolts<<','<<p.instantCellVolts<<','<<p.averagedCellVolts<<','<<p.filteredVolts<<','<<p.outputVolts<<','<<nodes[0]<<','<<nodes[1]<<','<<nodes[2]<<','<<(-j[0]+j[1]/cell::ar)<<','<<(j[2]-j[3]/cell::ar)<<','<<cell::norm(observed.residual)<<'\n';}}
 lab::require(!m.failures()&&!m.invalidInputs()&&!m.inputClamps()&&!m.tableClamps()&&!m.outputClamps(),"Containment cannot be called fault sound");
 m.reset();const auto cpu=std::clock();const auto start=std::chrono::steady_clock::now();for(std::size_t n=0;n<y.size();++n)repeat[n]=m.process(x.samples[n]);const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count(),cpuUs=1e6*double(std::clock()-cpu)/CLOCKS_PER_SEC;
 lab::require(repeat==y&&!m.failures()&&!m.tableClamps()&&!m.outputClamps(),"Exact reset/timed native repeat failed");f.cost<<label<<','<<fault<<','<<x.rate<<','<<y.size()<<','<<us<<','<<us/y.size()<<','<<cpuUs/y.size()<<','<<sizeof(m)<<'\n';
 return y;
}
void spectrum(const std::string& name,unsigned rate,const std::vector<float>& values,Files& f) {
 constexpr unsigned count=8192;lab::require(values.size()>=count,"Spectrum too short");const auto first=std::min<std::size_t>(rate,values.size()-count);std::vector<std::complex<double>> a(count);double sum=0;
 for(unsigned n=0;n<count;++n){const double w=.5-.5*std::cos(2*foldlab::pi*n/(count-1));a[n]=values[first+n]*w;sum+=w;}fft(a);
 for(unsigned n=0;n<=count/2;++n)f.spectra<<name<<','<<double(rate)*n/count<<','<<std::abs(a[n])*(n==0||n==count/2?1:2)/sum<<','<<rate<<','<<first<<','<<count<<'\n';
}
void save(const std::filesystem::path& out,const std::string& name,unsigned rate,const std::vector<float>& raw,double gain,double inputGain,float drive,bool fault,Files& f,const std::string& model="Wavefolder") {
 const auto before=soundforge::stats(raw);lab::require(gain>0&&gain<=16.00001,"Offline gain exceeds16");lab::Wave w{rate,raw};for(auto& a:w.samples)a=float(a*gain);const auto after=soundforge::stats(w.samples);lab::require(after.peak<.999,"Offline output clips");lab::write(out/name,w);
 f.levels<<name<<','<<model<<','<<rate<<','<<raw.size()<<','<<inputGain<<','<<drive<<','<<(model=="Wavefolder"?(fault?22000:15000):0)<<','<<(model=="RAT"||model=="CMOS"?4:model=="Muff"?9:8)<<','<<before.rms<<','<<before.peak<<','<<before.mean<<','<<gain<<','<<after.rms<<','<<after.peak<<','<<after.mean<<",0,0,0,0\n";
}
void group(const lab::Wave& x,const std::string& label,float drive,double calibration,const std::filesystem::path& out,Files& f,bool rms) {
 std::vector<std::vector<float>> signals(3);signals[0]=x.samples;for(auto& a:signals[0])a/=8;signals[1]=render(x,drive,false,label,f);signals[2]=render(x,drive,true,label,f);
 double common=16,target=soundforge::rmsTarget(signals,.1,.9);for(const auto& s:signals){const auto v=soundforge::stats(s);if(v.rms>0){common=std::min(common,.1/v.rms);target=std::min(target,16*v.rms);}if(v.peak>0)common=std::min(common,.9/v.peak);}
 for(unsigned j=0;j<3;++j){const std::string kind=j==0?"dry":j==1?"healthy":"RP22k";spectrum(label+"_"+kind,x.rate,signals[j],f);
  save(out,label+"_"+kind+"_raw.wav",x.rate,signals[j],1,calibration,drive,j==2,f,j==0?"dry":"Wavefolder");save(out,label+"_"+kind+"_common.wav",x.rate,signals[j],common,calibration,drive,j==2,f,j==0?"dry":"Wavefolder");
  if(rms)save(out,label+"_"+kind+"_rms_matched.wav",x.rate,signals[j],soundforge::rmsGain(soundforge::stats(signals[j]),target),calibration,drive,j==2,f,j==0?"dry":"Wavefolder");
 }
}
void comparison(const lab::Wave& x,double calibration,const std::filesystem::path& out,Files& f) {
 std::vector<std::vector<float>> s(5);s[0]=x.samples;for(auto& v:s[0])v/=8;s[1]=render(x,3,false,"comparison_wavefolder",f);for(unsigned k=2;k<5;++k)s[k].resize(x.samples.size());
 parable::dsp::Rat rat;parable::dsp::Muff muff;parable::dsp::Cmos cmos;
 lab::require(rat.configure(float(x.rate),{.5f,.5f,.5f})&&muff.prepare(float(x.rate),{false,.5f,.5f,.5f})&&cmos.configure(float(x.rate),{.5f,.5f}),"Main comparison models prepare failed");
 for(std::size_t n=0;n<x.samples.size();++n){s[2][n]=rat.process(x.samples[n]);s[3][n]=muff.process(x.samples[n]);s[4][n]=cmos.process(x.samples[n]);}
 lab::require(!muff.failures()&&!muff.inputClamps()&&!muff.outputClamps()&&!cmos.failedSolves(),"Existing comparison model containment");
 double target=soundforge::rmsTarget(s,.1,.9);for(const auto& v:s)target=std::min(target,16*soundforge::stats(v).rms);const char* labels[]={"dry","wavefolder","RAT","Muff","CMOS"};
 for(unsigned k=0;k<5;++k){const auto name=std::string("compare_")+labels[k];const char* model=k==1?"Wavefolder":labels[k];spectrum(name,x.rate,s[k],f);save(out,name+"_raw.wav",x.rate,s[k],1,calibration,k==1?3:.5f,false,f,model);save(out,name+"_rms_matched.wav",x.rate,s[k],soundforge::rmsGain(soundforge::stats(s[k]),target),calibration,k==1?3:.5f,false,f,model);}
}
std::vector<std::string> paths() {
 std::vector<std::string> r;for(const char* g:{"restrained","medium","intense"})for(const char* signal:{"dry","healthy","RP22k"})for(const char* mode:{"raw","common","rms_matched"})r.push_back(std::string(g)+"_"+signal+"_"+mode+".wav");
 for(const char* g:{"attacks","sustain","chord","sweep","two"})for(const char* signal:{"dry","healthy","RP22k"})for(const char* mode:{"raw","common"})r.push_back(std::string(g)+"_"+signal+"_"+mode+".wav");
 for(const char* g:{"dry","wavefolder","RAT","Muff","CMOS"})for(const char* mode:{"raw","rms_matched"})r.push_back(std::string("compare_")+g+"_"+mode+".wav");
 for(const char* name:{"renders.csv","state.csv","performance.csv","spectrum.csv","settings.json","LISTEN.txt"})r.push_back(name);
 return r;
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out,bool compare) {
 auto original=input.empty()?soundforge::syntheticPlucks():lab::read(input);lab::require(original.samples.size()>=original.rate&&original.samples.size()<=28*original.rate,"Input must be1–28s");lab::require(original.rate==44100||original.rate==48000||original.rate==96000,"Use44.1/48/96kHz");const auto originalStats=soundforge::stats(original.samples);lab::require(originalStats.peak>0,"Cannot calibrate silence");
 if(!input.empty())for(const auto& name:paths()){const auto p=out/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output aliases input source");}
 std::filesystem::create_directories(out);Files f;f.levels.open(out/"renders.csv");f.state.open(out/"state.csv");f.cost.open(out/"performance.csv");f.spectra.open(out/"spectrum.csv");for(auto* s:{&f.levels,&f.state,&f.cost,&f.spectra}){lab::require(bool(*s),"Cannot create receipt");*s<<std::setprecision(16);}
 f.levels<<"file,model,rate_hz,frames,input_gain_V_per_source_unit,condition_drive,upper_feed_ohms,software_output_divisor,raw_rms,raw_peak,raw_mean,offline_gain,final_rms,final_peak,final_mean,numerical_failures,input_clamps,table_clamps,output_clamps\n";
 f.state<<"group,fault,rate_hz,frame,last_phase3_s,input_V,cell_input_V,instant_readout_V,ADAA_readout_V,filtered_readout_V,output_AC_V,EP_V,EN_V,C_V,QP_IC_A,QN_IC_A,interpolated_KCL_peak_A\n";
 f.cost<<"group,fault,rate_hz,frames,full_process_wall_us,wall_us_per_frame,CPU_us_per_frame,state_bytes\n";f.spectra<<"group,hz,amplitude,rate_hz,start_frame,hann_frames\n";
 const double calibration=.2/originalStats.peak;auto calibrated=original;for(auto& a:calibrated.samples)a=float(a*calibration);
 const char* labels[]={"restrained","medium","intense"};const float drives[]={1,3,6};
 for(unsigned g=0;g<3;++g){auto x=calibrated;x.samples.resize(x.samples.size()+x.rate/5);group(x,labels[g],drives[g],calibration,out,f,true);}
 for(const std::string kind:{"attacks","sustain","chord","sweep","two"}){lab::Wave x{48000,{}};const unsigned length=kind=="sustain"?96000:kind=="attacks"?24000:57600;for(unsigned n=0;n<length;++n)x.samples.push_back(float(foldlab::stimulus(kind,double(n)/x.rate)));x.samples.resize(x.samples.size()+9600);group(x,kind,6,1,out,f,false);}
 if(compare){auto x=calibrated;x.samples.resize(x.samples.size()+2*x.rate);comparison(x,calibration,out,f);}
 for(auto* s:{&f.levels,&f.state,&f.cost,&f.spectra}){s->flush();lab::require(bool(*s),"Receipt write failed");}
 std::ofstream settings(out/"settings.json");settings<<"{\"experimental\":true,\"cells\":1,\"supply_V\":[-15,15],\"load_ohms\":10000,\"lower_feed_ohms\":15000,\"healthy_upper_feed_ohms\":15000,\"fault_upper_feed_ohms\":22000,\"T_K\":300,\"IS_A\":1e-17,\"BF\":100,\"BR\":1,\"input_peak_V\":0.2,\"drive\":[1,3,6],\"output_divisor\":8,\"coupling_C_F\":1e-7,\"coupling_R_ohms\":100000,\"oversampling\":4,\"ADAA\":true,\"FIR_taps\":193,\"lookup_intervals\":512,\"state_bytes\":"<<sizeof(Wavefolder)<<",\"offline_gain_ceiling\":16,\"main_tail_s\":0.2,\"comparison_tail_s\":2,\"comparisons\":"<<(compare?"true":"false")<<"}\n";
 std::ofstream listen(out/"LISTEN.txt");listen<<"SF014 one real loaded Lockhart cell; only upper feed15k->22k. Start medium_dry_common, medium_healthy_common, medium_RP22k_common; then *_rms_matched, restrained and intense. Common versions preserve raw fault loudness; RMS matches use disclosed individual gains, not equal gain/LUFS. Exact Centaur/tape take03 is intentional. Cell turnover, not flat clipping; literal sitar is not a target claim. Raw circuit DC (including~.794V fault) and currents remain in state.csv; common100n/100k output coupling blocks steady DC, precharged at reset. Raw musical files mean no offline gain; they retain normal coupling/FIR/ADAA/Volts8. Separate owned synthetic attacks/sustain/chord/sweep/twotone. Optional compare_* use the SAME phrase/calibration/2s tail through unchanged main RAT/Muff/CMOS; gain/filters/divisors differ, compare RMS-matched plus raw. RATDist/Filter/Volume.5; MuffSustain/Tone/Volume.5; CMOSDrive/Volume.5; wavefolderDrive3. See report for structural/spectral differences. No human/founder sound approval, physical transistor calibration or measured Seed3 performance.\n";
 settings.flush();listen.flush();lab::require(bool(settings)&&bool(listen),"Settings/listen write failed");
}
}
int main(int argc,char** argv)try {
 std::filesystem::path input,out;bool haveInput=false,haveOutput=false,compare=false;
 for(int i=1;i<argc;++i){const std::string a=argv[i];if(a=="--compare-existing"&&!compare){compare=true;continue;}lab::require(i+1<argc,"Missing option value");const std::string value=argv[++i];lab::require(!value.empty(),"Empty option value");if(a=="--input"&&!haveInput){input=value;haveInput=true;}else if(a=="--output"&&!haveOutput){out=value;haveOutput=true;}else throw std::runtime_error("Unknown/duplicate option");}
 lab::require(haveOutput,"Require --output DIR, optional --input monoPCM16.wav and --compare-existing");audition(input,out,compare);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
