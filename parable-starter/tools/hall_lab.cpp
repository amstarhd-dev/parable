#include "parable_hall.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
using parable::dsp::Hall;
struct Render {std::vector<float> left,right,mono;double us=0;uint32_t clips=0;};
Render render(const std::vector<float>&in,unsigned rate,float focus,float decay=1,float wet=.35f){
 auto hall=std::make_unique<Hall>();lab::require(hall->prepare(rate,{decay,wet,focus}),"prepare rejected");
 Render r;r.left.resize(in.size());r.right.resize(in.size());r.mono.resize(in.size());
 auto start=std::chrono::steady_clock::now();
 for(size_t i=0;i<in.size();i+=Hall::blockSize)hall->process(in.data()+i,r.left.data()+i,r.right.data()+i);
 r.us=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()*1e6/in.size();
 for(size_t i=0;i<in.size();++i)r.mono[i]=(r.left[i]+r.right[i])*.5f;
 r.clips=hall->containmentHits();return r;
}
// Host-only PCM16 stereo container. DSP/analysis/parser reuse existing tools.
void stereo(const std::filesystem::path&p,unsigned rate,const Render&r){
 auto l=lab::encode({rate,r.left}),rr=lab::encode({rate,r.right});std::vector<uint8_t>b(l.begin(),l.begin()+44);
 auto put=[&](size_t off,uint32_t v,int n){for(int i=0;i<n;++i)b[off+i]=uint8_t(v>>(8*i));};
 put(4,uint32_t(36+r.left.size()*4),4);put(22,2,2);put(28,rate*4,4);put(32,4,2);put(40,uint32_t(r.left.size()*4),4);
 for(size_t i=44;i<l.size();i+=2){b.push_back(l[i]);b.push_back(l[i+1]);b.push_back(rr[i]);b.push_back(rr[i+1]);}
 std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());lab::require(bool(f),"stereo write failed");
}
int main(int argc,char**argv){try{
 lab::require(argc==5 && std::string(argv[1])=="--input" && std::string(argv[3])=="--output","--input mono.wav --output DIR");
 auto wave=lab::read(argv[2]);lab::require(wave.rate==48000,"audition take must be48k");
 auto dir=std::filesystem::path(argv[4]);std::filesystem::create_directories(dir);
 wave.samples.resize(16*wave.rate);std::fill(wave.samples.begin()+8*wave.rate,wave.samples.end(),0);
 std::ofstream receipt(dir/"levels.csv");receipt<<std::setprecision(12)<<"name,rms,peak,mean,matching_gain,us_per_frame,containment_hits\n";
 auto save=[&](const std::string&name,const Render&r,double gain=1.){
  auto s=soundforge::stats(r.mono);receipt<<name<<','<<s.rms<<','<<s.peak<<','<<s.mean<<','<<gain<<','<<r.us<<','<<r.clips<<'\n';
  auto x=r.mono;for(auto&v:x)v*=gain;lab::write(dir/(name+".wav"),{wave.rate,x});
 };
 auto healthy=render(wave.samples,wave.rate,0),fault=render(wave.samples,wave.rate,.98f);
 save("healthy_raw",healthy);save("fault_raw",fault);
 auto target=soundforge::rmsTarget(std::vector<std::vector<float>>{healthy.mono,fault.mono},.05,.9);
 save("healthy_matched",healthy,soundforge::rmsGain(soundforge::stats(healthy.mono),target));save("fault_matched",fault,soundforge::rmsGain(soundforge::stats(fault.mono),target));
 stereo(dir/"healthy_stereo.wav",wave.rate,healthy);stereo(dir/"fault_stereo.wav",wave.rate,fault);
 save("healthy_wet",render(wave.samples,wave.rate,0,1,1));save("fault_wet",render(wave.samples,wave.rate,.98f,1,1));
 lab::write(dir/"dry.wav",wave);
 for(const std::string kind:{"impulse","chord","weak","silence"}){
  std::vector<float>in(wave.samples.size());
  if(kind=="weak"){in=wave.samples;for(auto&x:in)x*=.02f;}
  if(kind=="impulse")in[0]=.5f;
  if(kind=="chord")for(size_t i=0;i<2*wave.rate;++i){double t=double(i)/wave.rate;in[i]=float(.06*(1-std::exp(-t/.01))*(std::sin(2*3.141592653589793*110*t)+std::sin(2*3.141592653589793*138.59*t)+std::sin(2*3.141592653589793*164.81*t)));}
  for(float f:{0.f,.98f}){auto r=render(in,wave.rate,f,1,1);save(kind+(f==0?"_healthy":"_fault"),r);
   if(kind=="impulse"){std::ofstream b(dir/(f==0?"healthy_ir.f32":"fault_ir.f32"),std::ios::binary);b.write(reinterpret_cast<const char*>(r.mono.data()),r.mono.size()*4);}}
 }
 // Reset is a stopped operation. Explicit external5ms fade is part of this
 // audition protocol, not a claim of a live clickless reset implementation.
 auto hall=std::make_unique<Hall>();hall->prepare(wave.rate,{1,1,.98f});Render reset;reset.left.resize(wave.samples.size());reset.right.resize(wave.samples.size());reset.mono.resize(wave.samples.size());
 for(size_t i=0;i<wave.samples.size();i+=64){if(i==4*wave.rate)hall->reset();hall->process(wave.samples.data()+i,reset.left.data()+i,reset.right.data()+i);for(size_t j=0;j<64;++j){size_t n=i+j;float fade=std::min(1.f,std::abs(float(n)-4*wave.rate)/240.f);reset.mono[n]=fade*(reset.left[n]+reset.right[n])*.5f;}}
 save("reset_stopped_faded",reset);
 std::ofstream settings(dir/"settings.json");settings<<"{\"sample_rate\":48000,\"source_seconds\":8,\"tail_seconds\":8,\"wet\":0.35,\"decay\":1,\"healthy_focus\":0,\"fault_focus\":0.98,\"fixed_instance_bytes\":"<<sizeof(Hall)<<",\"core_bytes\":"<<Hall::stateBytes()<<",\"reset\":\"stopped, external5ms output fade\",\"synthetic_probes\":[\"impulse\",\"chord\",\"silence\"],\"matching\":\"whole-file RMS, not LUFS\"}\n";
 std::cout<<"Hall bytes="<<sizeof(Hall)<<" healthy_us="<<healthy.us<<" fault_us="<<fault.us<<"\n";
 }catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
