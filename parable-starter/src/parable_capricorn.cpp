#include "parable_capricorn.h"
#include "capricorn_circuit.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {constexpr double pi=3.14159265358979323846;bool unit(float v){return std::isfinite(v)&&v>=0&&v<=1;}void count(uint32_t& n){if(n!=std::numeric_limits<uint32_t>::max())++n;}
unsigned interval(const std::array<float,257>& x,float v){unsigned lo=0,hi=256;for(unsigned n=0;n<9&&hi-lo>1;++n){const auto m=(lo+hi)/2;if(x[m]<=v)lo=m;else hi=m;}return std::min(lo,255u);}
}
bool Capricorn::prepare(float rate,CapricornControls c,unsigned factor) noexcept {
    if(!std::isfinite(rate)||rate<44100||rate>96000||!unit(c.distortion)||!unit(c.tone)||!unit(c.level)||(factor!=1&&factor!=4&&factor!=8))return false;
    Capricorn next;next.factor_=factor;next.taps_=24*factor+1;const double fs=rate*factor;capricorn_detail::Q2 q;
    if(!q.solve(q.bias,0,true))return false;
    next.bias_=q.bias;
    auto point=[&](unsigned k,capricorn_detail::V<3>& v){const double u=(double(k)-128)/128,x=.5*u*u*u;if(!q.solve(v,x))return false;capricorn_detail::V<3> rhs{},d{};rhs[0]=1./550;if(!capricorn_detail::linear<3>(q.jacobian(v),rhs,d))return false;
        next.qx_[k]=float(x);next.qy_[k]=float(v[1]-q.bias[1]);next.qs_[k]=float(d[1]);return true;};
    auto v=q.bias;for(unsigned k=128;k<257;++k)if(!point(k,v))return false;v=q.bias;for(int k=127;k>=0;--k)if(!point(unsigned(k),v))return false;
    for(unsigned k=129;k<257;++k){const float h=next.qx_[k]-next.qx_[k-1];next.qF_[k]=next.qF_[k-1]+h*((next.qy_[k]+next.qy_[k-1])*.5f+h*(next.qs_[k-1]-next.qs_[k])/12);}
    for(int n=127;n>=0;--n){const auto k=unsigned(n);const float h=next.qx_[k+1]-next.qx_[k];next.qF_[k]=next.qF_[k+1]-h*((next.qy_[k]+next.qy_[k+1])*.5f+h*(next.qs_[k]-next.qs_[k+1])/12);}
    next.inverseGain_=1/next.qs_[128];const auto j=q.jacobian(q.bias);capricorn_detail::M<4> g{};for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)g[a][b]=j[a][b];g[0][0]-=1./550;g[3][3]=1./550;
    capricorn_detail::V<4> drive{};drive[3]=1./550;std::array<capricorn_detail::V<4>,2> t{};t[0][3]=1;t[0][0]=-1;t[1][0]=1;t[1][1]=-1;
    if(!capricorn_detail::compile<4,2>(g,drive,t,{47e-9,c.openC4?0:250e-12},fs,1,1,next.front_)||!capricorn_detail::tone(fs,c,next.post_))return false;
    for(unsigned k=0;k<257;++k){const double u=double(k)/256,x=16*u*u*u;double lo=0,hi=1.2;for(unsigned n=0;n<64;++n){const double y=(lo+hi)/2;if(y+next.post_.resistance*2*capricorn_detail::diodeIs*std::sinh(y/capricorn_detail::nv)>x)hi=y;else lo=y;}
        const double y=k?(lo+hi)/2:0;next.dx_[k]=float(x);next.dy_[k]=float(y);next.ds_[k]=float(1/(1+next.post_.resistance*2*capricorn_detail::diodeIs/capricorn_detail::nv*std::cosh(y/capricorn_detail::nv)));}
    auto rc=[&](capricorn_detail::RC& r,double tau){r.g=float(1/(1+2*fs*tau));};rc(next.input_,472000*10e-9);rc(next.coupling_,100000*.47e-6);
    const double upper=100000*c.distortion,lower=100000*(1-c.distortion)+4700;next.extraGain_=float(upper/lower);rc(next.gain_,lower*1e-6);rc(next.amplifier_,1/(2*pi*20000));rc(next.output1_,1e6*47e-9);rc(next.output2_,(100000/1.1)*1e-6);
    double sum=0;const unsigned center=(next.taps_-1)/2;const double cutoff=.4/factor;for(unsigned n=0;n<=center;++n){const double x=double(n)-center;next.fir_[n]=float((x==0?2*cutoff:std::sin(2*pi*cutoff*x)/(pi*x))*(.42-.5*std::cos(2*pi*n/(next.taps_-1))+.08*std::cos(4*pi*n/(next.taps_-1))));sum+=next.fir_[n]*(n==center?1:2);}for(auto& x:next.fir_)x=float(x/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
Capricorn::Point Capricorn::qValue(float x) const noexcept {const auto k=interval(qx_,x);const float h=qx_[k+1]-qx_[k],t=(x-qx_[k])/h,a=qy_[k],d=h*qs_[k],b=3*(qy_[k+1]-a)-2*d-h*qs_[k+1],c=2*(a-qy_[k+1])+d+h*qs_[k+1];return {a+t*(d+t*(b+t*c)),qF_[k]+h*t*(a+t*(d/2+t*(b/3+t*c/4)))};}
float Capricorn::transistorCurve(float x) const noexcept {return prepared_&&std::isfinite(x)?qValue(std::clamp(x,-.5f,.5f)).y:0;}
float Capricorn::diodeCurve(float x) const noexcept {if(!prepared_||!std::isfinite(x))return 0;const float a=std::min(std::abs(x),16.f);const auto k=interval(dx_,a);const float h=dx_[k+1]-dx_[k],t=(a-dx_[k])/h,y=dy_[k],d=h*ds_[k],b=3*(dy_[k+1]-y)-2*d-h*ds_[k+1],c=2*(y-dy_[k+1])+d+h*ds_[k+1];return std::copysign(y+t*(d+t*(b+t*c)),x);}
void Capricorn::reset() noexcept {front_.state.fill(0);post_.state.fill(0);input_.z=coupling_.z=gain_.z=amplifier_.z=output1_.z=output2_.z=previous_=previousArea_=0;up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;failures_=invalid_=clamps_=tableClamps_=0;probe_={};running_=prepared_;}
float Capricorn::sample(float input) noexcept {
    const float buffered=input_.high(input)*(470000.f/472000);float x=front_.step(buffered)*inverseGain_;
    if(std::abs(x)>.5f){count(tableClamps_);x=std::clamp(x,-.5f,.5f);}const auto v=qValue(x);const float dx=x-previous_;
    const float q2=std::abs(dx)<1e-6f?qValue(.5f*(x+previous_)).y:(v.F-previousArea_)/dx;previous_=x;previousArea_=v.F;
    const float coupled=coupling_.high(q2),request=coupled+extraGain_*gain_.high(coupled),amp=std::clamp(amplifier_.low(std::clamp(request,-3.f,3.f)),-3.f,3.f);
    const float source=post_.source(amp);if(std::abs(source)>16)count(tableClamps_);const float diode=diodeCurve(source),jack=output2_.high(output1_.high(post_.step(amp,source-diode)));
    probe_={q2,amp,diode,jack};return jack/2;
}
float Capricorn::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}if(std::abs(input)>.5f){count(clamps_);input=std::clamp(input,-.5f,.5f);}
    float output=0;if(factor_==1)output=sample(input);else {upIndex_=upIndex_?upIndex_-1:24;up_[upIndex_]=input;
        for(unsigned phase=0;phase<factor_;++phase){float x=0;unsigned j=upIndex_;for(unsigned tap=phase;tap<taps_;tap+=factor_){x+=coefficient(tap)*up_[j];if(++j==25)j=0;}
            downIndex_=downIndex_?downIndex_-1:taps_-1;down_[downIndex_]=sample(float(factor_)*x);if(phase==0){j=downIndex_;for(unsigned tap=0;tap<taps_;++tap){output+=coefficient(tap)*down_[j];if(++j==taps_)j=0;}}}}
    if(!std::isfinite(output)||std::abs(output)>1){count(failures_);return std::isfinite(output)?std::clamp(output,-1.f,1.f):0;}return output;
}
}
