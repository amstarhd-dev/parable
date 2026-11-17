#include "wallace_graph.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
int main(){
    activation_guard::selfTest();wallace::RoutingConfig c;wallace::Routing route;
    CHECK(route.prepare(c));CHECK(sizeof(route)<512);
    for(float x:{-1.f,6.01f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){auto bad=c;bad.duckDb=x;CHECK(!route.prepare(bad));}
    c.clean=1;c.core=c.rotary=c.tape=c.hall=0;c.ducking=false;CHECK(route.prepare(c));
    bool delayCorrect=true;for(unsigned i=0;i<100;++i){const float y=route.process(i==0?1.f:0,0,0,0,0).output;delayCorrect&=y==(i==48?1.f:0);}CHECK(delayCorrect);
    c={};c.low=0;c.high=1;c.carving=false;CHECK(route.prepare(c));
    wallace::Returns y;for(int i=0;i<480;++i)y=route.process(1,0,0,1,1);
    CHECK(std::abs(y.envelope-(1-std::exp(-1.)))<.0001);const float held=y.envelope;
    for(int i=0;i<12000;++i)y=route.process(0,0,0,1,1);
    CHECK(std::abs(y.envelope-held*std::exp(-1.))<.0002);
    double min=1,max=0,maxStep=0;float previous=1;
    CHECK(route.prepare(c));activation_guard::begin();
    for(int i=0;i<96000;++i){auto r=route.process(i<48000?1.f:0,.3f,.2f,.4f,.5f);min=std::min(min,double(r.gain));max=std::max(max,double(r.gain));maxStep=std::max(maxStep,double(std::abs(r.gain-previous)));previous=r.gain;}
    activation_guard::end();CHECK(min>=std::pow(10.,-4./20)-1e-6&&max<=1);CHECK(maxStep<.001);
    c.duckDb=0;CHECK(route.prepare(c));CHECK(route.process(1,1,1,1,1).gain==1);
    c={};c.ducking=false;CHECK(route.prepare(c));for(int i=0;i<96000;++i)y=route.process(0,0,0,1,1);CHECK(std::abs(y.output)<.0002);
    c.carving=false;CHECK(route.prepare(c));CHECK(std::abs(route.process(0,0,0,1,1).output-.4f)<1e-7);
    c={};c.ducking=false;c.tape=c.hall=1;CHECK(route.prepare(c));
    double ei=0,et=0,eh=0;
    for(unsigned i=0;i<48000;++i){const float t=float(std::sin(2*3.141592653589793*120*i/48000)),h=float(std::sin(2*3.141592653589793*180*i/48000));const auto r=route.process(0,0,0,t,h);if(i>=24000){ei+=double(t)*t;et+=double(r.tape)*r.tape;eh+=double(r.hall)*r.hall;}}
    CHECK(std::abs(std::sqrt(et/ei)-std::sqrt(.5))<.001);CHECK(std::abs(std::sqrt(eh/ei)-std::sqrt(.5))<.001);
    c={};CHECK(route.prepare(c));auto first=route.process(.2,.1,.3,.4,.5);
    auto noDuck=c;noDuck.ducking=false;wallace::Routing staticRoute;CHECK(staticRoute.prepare(noDuck));const auto staticFirst=staticRoute.process(.2,.1,.3,.4,.5);
    CHECK(first.core==staticFirst.core&&first.rotary==staticFirst.rotary);CHECK(first.gain<=1);
    //Actual canonical processors: C-no-duck == B over audible state/tails.
    auto b=std::make_unique<wallace::Graph>(),d=std::make_unique<wallace::Graph>();
    CHECK(b->prepare(wallace::Mode::Static));CHECK(d->prepare(wallace::Mode::Dynamic,noDuck));
    std::array<float,64> in{};wallace::Block a,z;bool same=true,finite=true;double energy=0;
    activation_guard::begin();
    for(size_t n=0;n<3*48000;n+=64){for(size_t j=0;j<64;++j){double t=double(n+j)/48000;in[j]=t<1?float(.1*std::sin(2*3.141592653589793*131*t)):0;}
        if(!b->process(in.data(),a)||!d->process(in.data(),z))same=false;
        same&=a.out==z.out;for(float x:a.out){finite&=std::isfinite(x);energy+=double(x)*x;}}
    activation_guard::end();CHECK(same);CHECK(finite&&energy>.01);CHECK(b->healthy()&&d->healthy());
    in.fill(0);in[63]=std::numeric_limits<float>::quiet_NaN();CHECK(!b->process(in.data(),a));CHECK(std::all_of(a.out.begin(),a.out.end(),[](float x){return x==0;}));
    in.fill(0);CHECK(b->process(in.data(),a));CHECK(d->process(in.data(),z));CHECK(a.out==z.out);
    CHECK(activation_guard::clean());std::cout<<testChecks<<" Wallace checks: controls/HPF/gain bounds/48sample clean alignment, actual C-no-duck identity and callback guards\n";
}
