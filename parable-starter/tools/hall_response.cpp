// Offline actual native impulse capture at supported rates/decay endpoints.
#include "parable_hall.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
int main(int argc,char**argv){if(argc!=2)return 2;std::filesystem::path out=argv[1];std::filesystem::create_directories(out);
 auto hall=std::make_unique<parable::dsp::Hall>();std::array<float,64>in{},left{},right{},mono{};
 for(unsigned rate:{44100u,48000u,96000u})for(float decay:{0.f,1.f}){
  if(!hall->prepare(rate,{decay,1,0}))return 3;
  std::ofstream f(out/(std::to_string(rate)+"-"+std::to_string(int(decay))+".f32"),std::ios::binary);
  // Round capture length to complete blocks; no unchecked partial callback.
  for(size_t n=0;n<16*rate;n+=64){in.fill(0);if(n==0){in[0]=.5f;in[1]=.5f;}hall->process(in.data(),left.data(),right.data());for(size_t j=0;j<64;++j)mono[j]=(left[j]+right[j])*.5f;f.write(reinterpret_cast<const char*>(mono.data()),sizeof(mono));}
  if(!f || hall->containmentHits())return 4;
 }
}
