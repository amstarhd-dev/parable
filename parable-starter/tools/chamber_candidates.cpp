// SF021 research-only: calls complete pinned upstream cores without rewriting DSP.
#include <algorithm>
#include <array>
#include <memory>
#include <iostream>
#include <chrono>
#include <malloc.h>
#include <iomanip>
#include "activation_guard.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#if ENGINE_CLOUD
#include "cloudseed/DSP/ReverbController.h"
#include "cloudseed/Programs.h"
#else
#include "basics/include/signalsmith-basics/reverb.h"
#endif
int main(int argc,char**argv){try{
 lab::require(argc==4,"input output mode(default/preset/compact/envelope)");auto input=lab::read(argv[1]);const unsigned fs=input.rate;const std::string mode=argv[3];lab::require(mode=="default"||mode=="preset"||mode=="compact"||mode=="envelope","Unknown preset");lab::require(fs==44100||fs==48000||fs==96000,"Unsupported rate");
 input.samples.resize(std::min(input.samples.size(),std::size_t(fs*8)));const double gain=.15/soundforge::stats(input.samples).peak;for(auto&x:input.samples)x=float(x*gain);input.samples.resize(fs*14,0);
 activation_guard::selfTest();
 std::filesystem::path output=argv[2];std::filesystem::create_directories(output);const unsigned block=64;std::vector<float> result(input.samples.size()),left(block),right(block);float* out[]={left.data(),right.data()};
auto before=mallinfo2();activation_guard::preparation=true;
 auto create=[&](){
#if ENGINE_CLOUD
 std::srand(1);auto e=std::make_unique<Cloudseed::ReverbController>(fs);Cloudseed::initPrograms();
 for(int i=0;i<Cloudseed::Parameter::COUNT;++i)e->SetParameter(i,Cloudseed::ProgramDarkPlate[i]);
 if(mode=="compact") {using namespace Cloudseed;e->SetParameter(Parameter::LateLineDecay,.434840339831037);e->SetParameter(Parameter::LateLineSize,.2);e->SetParameter(Parameter::HighCutEnabled,1);e->SetParameter(Parameter::HighCut,std::log2(1+15*(6500.-400)/19600)/4);e->SetParameter(Parameter::LowCut,std::log2(1+15*(120.-20)/980)/4);e->SetParameter(Parameter::TapEnabled,1);e->SetParameter(Parameter::TapCount,.035);e->SetParameter(Parameter::TapLength,.08);e->SetParameter(Parameter::EarlyOut,.5);}
 if(mode=="envelope"){using namespace Cloudseed;e->SetParameter(Parameter::LateLineDecay,.25);e->SetParameter(Parameter::LateLineSize,.2);e->SetParameter(Parameter::HighCutEnabled,1);e->SetParameter(Parameter::HighCut,std::log2(1+15*(6500.-400)/19600)/4);e->SetParameter(Parameter::LowCut,std::log2(1+15*(120.-20)/980)/4);e->SetParameter(Parameter::TapEnabled,1);e->SetParameter(Parameter::TapCount,.035);e->SetParameter(Parameter::TapLength,.08);e->SetParameter(Parameter::EarlyOut,.5);}
 if(mode!="preset")e->SetParameter(Cloudseed::Parameter::DryOut,0);
 e->ClearBuffers();
#else
 auto e=std::make_unique<signalsmith::basics::ReverbFloat>();
 if(mode=="preset"){auto select=[](const char*name){return std::string(name)=="ambient";};e->presets(select);}
 if(mode=="compact"){e->roomMs=28;e->rt20=.4;e->early=1.5;e->detune=.5;e->lowCutHz=120;e->highCutHz=6500;e->lowDampRate=1.5;e->highDampRate=2.5;}
 if(mode=="envelope"){e->roomMs=28;e->rt20=.83;e->early=1.5;e->detune=.5;e->lowCutHz=120;e->highCutHz=6500;e->lowDampRate=1.5;e->highDampRate=2.5;}
 e->dry=0;e->wet=1;lab::require(e->configure(fs,block,2),"configure failed");e->reset();
#endif
 return e;};auto e=create();
 activation_guard::preparation=false;auto after=mallinfo2();
 std::ofstream receipt(output/(mode+"_receipt.json"));receipt<<std::setprecision(12)<<"{\"rate\":"<<fs<<",\"input_gain\":"<<gain<<",\"core_bytes\":"<<sizeof(*e)<<",\"observed_allocator_delta_bytes\":"<<((after.uordblks+after.hblkhd)-(before.uordblks+before.hblkhd))<<",\"cumulative_preparation_cpp_bytes\":"<<activation_guard::control.bytes;
 auto start=std::chrono::steady_clock::now();activation_guard::begin();for(std::size_t n=0;n<input.samples.size();n+=block){const unsigned len=std::min(std::size_t(block),input.samples.size()-n);float* in[]={input.samples.data()+n,input.samples.data()+n};
#if ENGINE_CLOUD
 e->Process(in[0],in[1],out[0],out[1],len);
#else
 e->process(in,out,len);
#endif
 for(unsigned j=0;j<len;++j)result[n+j]=.5f*(left[j]+right[j]);}
 activation_guard::end();lab::require(activation_guard::clean(),"Upstream processing allocated/locked/IO");
 const double cost=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/input.samples.size();
 std::cout<<"sizeof="<<sizeof(*e)<<" us/frame="<<cost<<" rms="<<soundforge::stats(result).rms<<" peak="<<soundforge::stats(result).peak<<"\n";
 lab::write(output/(mode+"_upstream_raw.wav"),{fs,result});
 const double target=soundforge::rmsTarget(std::vector<std::vector<float>>{result},.04,.85);
 const double match=soundforge::rmsGain(soundforge::stats(result),target);
 auto matched=result;for(auto&x:matched)x=float(x*match);
 lab::write(output/(mode+"_matched.wav"),{fs,matched});
 if(mode=="compact") {
  auto mix=result;for(std::size_t i=0;i<mix.size();++i)mix[i]=.35f*input.samples[i]+.65f*matched[i];
  lab::write(output/"compact_comparable_mix.wav",{fs,mix});
  lab::write(output/"dry_common.wav",input);
 }
 receipt<<",\"us_per_frame\":"<<cost<<",\"raw_rms\":"<<soundforge::stats(result).rms<<",\"raw_peak\":"<<soundforge::stats(result).peak<<",\"matched_gain\":"<<match<<",\"matched_target_rms\":"<<target<<",\"callback_guard_clean\":true}\n";
 // Reconstruct for a fresh impulse; no retained phase/filter/pointer history.
 e.reset();e=create();
 lab::Wave ir{fs,std::vector<float>(fs*6)};std::vector<float> impulse(ir.samples.size());impulse[0]=.5f;
 activation_guard::begin();for(std::size_t n=0;n<impulse.size();n+=block){unsigned len=std::min(std::size_t(block),impulse.size()-n);float*in[]={impulse.data()+n,impulse.data()+n};
#if ENGINE_CLOUD
 e->Process(in[0],in[1],out[0],out[1],len);
#else
 e->process(in,out,len);
#endif
 for(unsigned j=0;j<len;++j)ir.samples[n+j]=.5f*(left[j]+right[j]);}
 activation_guard::end();lab::require(activation_guard::clean(),"Impulse callback guard failed");lab::write(output/(mode+"_impulse.wav"),ir);
 std::ofstream floats(output/(mode+"_impulse.f32"),std::ios::binary);floats.write(reinterpret_cast<const char*>(ir.samples.data()),ir.samples.size()*sizeof(float));lab::require(bool(floats),"Cannot write float impulse");

 return 0;}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
