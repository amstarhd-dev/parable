#include "wallace2_graph.h"
#include "activation_guard.h"
#include "test_check.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <cstring>
std::vector<float> readKernel(const std::string& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);CHECK(bool(f));const auto n=f.tellg();CHECK(n>0&&n%4==0&&n<=32768*4);
    std::vector<float> h(size_t(n)/4);f.seekg(0);for(auto&x:h){unsigned char b[4];f.read(reinterpret_cast<char*>(b),4);uint32_t u=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);std::memcpy(&x,&u,4);}CHECK(bool(f));return h;
}
// Independent O(N*M) double time-domain reference; no FFT or borrowed code.
void reference(const std::vector<float>& h){
    wallace2::Cabinet c;CHECK(c.prepare(48000,h.data(),h.size()));
    const size_t frames=h.size()+512;std::vector<float> x(frames),y(frames),other(frames);uint32_t state=123;
    for(size_t i=0;i<257;++i){state=1664525*state+1013904223;x[i]=float(int(state>>16)-32768)/327680;}
    size_t pos=0;activation_guard::begin();
    while(pos<frames){const auto n=std::min<size_t>((pos%63)+1,frames-pos);if(!c.process(x.data()+pos,y.data()+pos,n))std::abort();pos+=n;}
    activation_guard::end();double maxError=0,squared=0,energy=0;
    for(size_t n=0;n<frames;++n){double sum=0;for(size_t i=0;i<257&&i<=n;++i)if(n-i<h.size())sum+=double(x[i])*h[n-i];const double e=y[n]-sum;maxError=std::max(maxError,std::abs(e));squared+=e*e;energy+=sum*sum;}
    // Includes upstream <1e-6 tail trimming, not merely the effective trimmed IR.
    CHECK(maxError<8e-6);CHECK(std::sqrt(squared/energy)<2e-4);
    CHECK(c.prepare(48000,h.data(),h.size()));
    for(pos=0;pos<frames;pos+=64)CHECK(c.process(x.data()+pos,other.data()+pos,std::min<size_t>(64,frames-pos)));
    double different=0;for(size_t i=0;i<frames;++i)different=std::max(different,double(std::abs(y[i]-other[i])));CHECK(different<1e-7);
}
int main(int argc,char**argv){
    CHECK(argc==2);activation_guard::selfTest();
    std::vector<float> h(257);for(size_t i=0;i<h.size();++i)h[i]=float(std::cos(i*.17)*std::exp(-double(i)*.02)*.05);reference(h);
    for(const std::string name:{"jensen","v30"}){
        const auto ir=readKernel(std::string(argv[1])+"/"+name+"-48k.f32");reference(ir);
        activation_guard::control={};activation_guard::preparation=true;
        auto measured=std::make_unique<wallace2::Cabinet>();const bool ok=measured->prepare(48000,ir.data(),ir.size());
        activation_guard::preparation=false;CHECK(ok);
        std::cout<<name<<" cab creation+prepare requested bytes "<<activation_guard::control.bytes<<", allocation calls "<<activation_guard::control.cppAlloc<<" (gross, not live heap/high-water)\n";
    }
    wallace2::Cabinet c;std::array<float,64> in{},out{};const float delta=1;
    CHECK(!c.process(in.data(),out.data()));CHECK(!c.prepare(44100,&delta,1));CHECK(!c.prepare(48000,nullptr,1));CHECK(!c.prepare(48000,&delta,0));CHECK(!c.prepare(48000,&delta,32769));
    const float bad=std::numeric_limits<float>::quiet_NaN(),zero=0;CHECK(!c.prepare(48000,&bad,1));CHECK(!c.prepare(48000,&zero,1));CHECK(c.prepare(48000,&delta,1));
    in[0]=.5;CHECK(c.process(in.data(),out.data()));CHECK(out[0]==.5f);for(size_t i=1;i<64;++i)CHECK(std::abs(out[i])<1e-7);
    in.fill(0);in[63]=bad;CHECK(!c.process(in.data(),out.data()));in.fill(0);CHECK(c.process(in.data(),out.data()));CHECK(std::all_of(out.begin(),out.end(),[](float x){return std::abs(x)<1e-7;}));
    CHECK(!c.process(in.data(),out.data(),65));CHECK(!c.process(in.data(),in.data()));
    auto on=std::make_unique<wallace2::Graph>(),off=std::make_unique<wallace2::Graph>(),post=std::make_unique<wallace2::Graph>();wallace2::Config cfg;
    CHECK(cfg.clean==.5f&&cfg.muff==.75f);auto invalid=cfg;invalid.clean=0;CHECK(!on->prepare(&delta,1,invalid));invalid.clean=bad;CHECK(!on->prepare(&delta,1,invalid));
    CHECK(on->prepare(&delta,1,cfg));cfg.clean=0;cfg.cleanDisabledControl=true;CHECK(off->prepare(&delta,1,cfg));cfg={};cfg.placement=wallace2::Placement::PostCab;CHECK(post->prepare(&delta,1,cfg));
    wallace2::Block a,b,p;std::array<float,48> delay{};size_t head=0;bool exact=true;double cleanPower=0,totalPower=0;
    activation_guard::begin();
    for(size_t n=0;n<48000;n+=64){for(size_t j=0;j<64;++j)in[j]=float(.1*std::sin(2*3.141592653589793*131*(n+j)/48000));
        exact&=on->process(in.data(),a)&&off->process(in.data(),b)&&post->process(in.data(),p);
        for(size_t j=0;j<64;++j){const float clean=.5f*delay[head];delay[head]=in[j];if(++head==48)head=0;
            exact&=a.clean[j]==clean&&p.clean[j]==clean&&b.clean[j]==0;
            exact&=std::abs((a.out[j]-b.out[j])-clean)<2e-7f;
            exact&=a.wet[j]==b.wet[j]&&a.fuzz[j]==b.fuzz[j];
            exact&=std::abs(p.out[j]-(p.clean[j]+p.fuzz[j]+p.wet[j]))<2e-7f;
            exact&=p.duck[j]==1;cleanPower+=double(clean)*clean;totalPower+=double(a.out[j])*a.out[j];}}
    activation_guard::end();CHECK(exact);CHECK(cleanPower>.01&&cleanPower/totalPower>.05);CHECK(on->healthy()&&off->healthy()&&post->healthy());
    in[63]=bad;CHECK(!on->process(in.data(),a));CHECK(std::all_of(a.out.begin(),a.out.end(),[](float x){return x==0;}));
    CHECK(activation_guard::clean());std::cout<<testChecks<<" checks: independent convolution, chunks/tails/reset, zero buffering latency, real clean/stem routing and callback guards\n";
}
