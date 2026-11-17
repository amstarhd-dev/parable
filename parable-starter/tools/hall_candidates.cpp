#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <iostream>
#include <malloc.h>
#include <memory>
#include <fstream>
#ifdef CATHEDRAL
#include "kCathedral.h"
using Core=kCathedral;
#else
#include "Verbity.h"
using Core=Verbity;
#endif
int main(int argc,char**argv){try{
 lab::require((argc>=3 && argc<=5),"input output [Verbity longness] [bigness]");auto wave=lab::read(argv[1]);wave.samples.resize(16*wave.rate);for(size_t i=8*wave.rate;i<wave.samples.size();++i)wave.samples[i]=0;
 auto before=mallinfo2().uordblks;std::srand(7);auto core=std::make_unique<Core>(nullptr);auto after=mallinfo2().uordblks; core->setSampleRate(wave.rate);
#ifdef CATHEDRAL
 core->setParameter(0,1.f);
#else
 core->setParameter(0,argc==5?std::stof(argv[4]):.8f);core->setParameter(1,argc>=4?std::stof(argv[3]):.85f);core->setParameter(2,.3f);core->setParameter(3,1.f);
#endif
 std::vector<float>l(wave.samples.size()),r(l.size()); auto start=std::chrono::steady_clock::now();
 for(size_t i=0;i<l.size();i+=64){float*ins[]={wave.samples.data()+i,wave.samples.data()+i};float*outs[]={l.data()+i,r.data()+i};core->processReplacing(ins,outs,std::min(size_t(64),l.size()-i));}
 auto sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();auto output=std::filesystem::path(argv[2]);std::filesystem::create_directories(output);
 std::vector<float>mono(l.size()),mix(l.size());for(size_t i=0;i<l.size();++i){mono[i]=(l[i]+r[i])*.5f;mix[i]=.65f*wave.samples[i]+.35f*mono[i];}
 for(auto pair:{std::make_pair("wet",&mono),std::make_pair("mix",&mix),std::make_pair("left",&l),std::make_pair("right",&r)}){auto values=*pair.second;auto s=soundforge::stats(values);std::cout<<pair.first<<" rms="<<s.rms<<" peak="<<s.peak<<"\n";lab::write(output/(std::string(pair.first)+".wav"),{wave.rate,values});if(std::string(pair.first)=="mix"){auto g=soundforge::rmsGain(s,.05);for(auto &x:values)x*=g;lab::write(output/"matched.wav",{wave.rate,values});}}
 std::cout<<"sizeof="<<sizeof(Core)<<" alloc_delta="<<after-before<<" microseconds_per_frame="<<sec*1e6/l.size()<<"\n";
 std::srand(7);core=std::make_unique<Core>(nullptr);core->setSampleRate(wave.rate);
#ifdef CATHEDRAL
 core->setParameter(0,1.f);
#else
 core->setParameter(0,argc==5?std::stof(argv[4]):.8f);core->setParameter(1,argc>=4?std::stof(argv[3]):.85f);core->setParameter(2,.3f);core->setParameter(3,1.f);
#endif
 std::fill(wave.samples.begin(),wave.samples.end(),0);wave.samples[0]=.5;
 for(size_t i=0;i<l.size();i+=64){float*ins[]={wave.samples.data()+i,wave.samples.data()+i};float*outs[]={l.data()+i,r.data()+i};core->processReplacing(ins,outs,std::min(size_t(64),l.size()-i));}
 std::ofstream f(output/"impulse.f32",std::ios::binary);for(size_t i=0;i<l.size();++i){float x=(l[i]+r[i])*.5;f.write(reinterpret_cast<char*>(&x),4);}
 }catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
