#include "parable_scorpio.h"
#include "scorpio_circuit.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {constexpr double pi=3.14159265358979323846;
void count(uint32_t& n) noexcept {if(n!=std::numeric_limits<uint32_t>::max())++n;}
bool unit(float n) noexcept {return std::isfinite(n)&&n>=0&&n<=1;}}
bool Scorpio::prepare(float rate,ScorpioControls c,unsigned factor,bool saturating,bool adaa) noexcept {
    if(!std::isfinite(rate)||rate<44100||rate>(factor==1?384000:96000)||!unit(c.gain)||!unit(c.lowCut)||!unit(c.level)||(factor!=1&&factor!=4))return false;
    Scorpio next;next.controls_=c;next.factor_=factor;next.saturating_=saturating;next.adaa_=adaa;next.dt_=1/(rate*factor);
    scorpio_detail::Circuit core;core.r3=c.resistorDrift?10000:5600;core.feedback=5600+250000*c.gain;core.lowResistance=1+9999*c.lowCut;
    if(!core.solve(core.bias,0,true))return false;
    next.bias_=core.bias;
    auto makePoint=[&](unsigned i,scorpio_detail::Vec& previous){const double u=(double(i)-256)/256,x=4*u*u*u;
        if(!core.solve(previous,x))return false;
        scorpio_detail::Vec derivative{},rhs{};rhs[0]=.001;
        if(!scorpio_detail::linear<5>(core.jacobian(previous),rhs,derivative))return false;
        next.x_[i]=float(x);next.y_[i]=float(previous[4]-core.bias[4]);next.slope_[i]=float(derivative[4]);return true;};
    auto previous=core.bias;for(unsigned i=256;i<513;++i)if(!makePoint(i,previous))return false;
    previous=core.bias;for(int i=255;i>=0;--i)if(!makePoint(unsigned(i),previous))return false;
    for(unsigned i=257;i<513;++i){const float h=next.x_[i]-next.x_[i-1];next.area_[i]=next.area_[i-1]+h*((next.y_[i]+next.y_[i-1])*.5f+h*(next.slope_[i-1]-next.slope_[i])/12);}
    for(int i=255;i>=0;--i){const auto j=unsigned(i);const float h=next.x_[j+1]-next.x_[j];next.area_[j]=next.area_[j+1]-h*((next.y_[j]+next.y_[j+1])*.5f+h*(next.slope_[j]-next.slope_[j+1])/12);}
    const auto j=core.jacobian(core.bias);scorpio_detail::Vec source{},resistance{};source[4]=1;
    if(!scorpio_detail::linear<5>(j,source,resistance)||resistance[4]<=0)return false;
    next.sourceOhms_=float(resistance[4]);next.inverseGain_=1/next.slope_[256];
    // Exact bilinear small-signal C2/C3/C7/C8/C9 network, reduced off-thread to
    // five capacitor-voltage histories. Nonlinear large-signal charge is omitted.
    scorpio_detail::Matrix<7> g{};for(unsigned a=0;a<5;++a)for(unsigned b=0;b<5;++b)g[a][b]=j[a][b];
    g[0][0]-=.001;g[6][6]=.001;g[3][3]-=1/core.lowResistance;
    const auto resistor=[&](unsigned a,unsigned b,double conductance){g[a][a]+=conductance;g[b][b]+=conductance;g[a][b]-=conductance;g[b][a]-=conductance;};
    resistor(3,5,1/core.lowResistance);
    const unsigned a[]={6,0,4,3,5};const int b[]={0,-1,2,-1,-1};const double caps[]={1e-6,330e-12,1e-9,330e-9,330e-6};
    double cg[5];for(unsigned k=0;k<5;++k){cg[k]=2*rate*factor*caps[k];g[a[k]][a[k]]+=cg[k];if(b[k]>=0){const auto q=unsigned(b[k]);g[q][q]+=cg[k];g[a[k]][q]-=cg[k];g[q][a[k]]-=cg[k];}}
    scorpio_detail::Vector<7> drive{},d{};drive[6]=.001;if(!scorpio_detail::linear<7>(g,drive,d))return false;
    next.D_=float(d[4]);for(unsigned k=0;k<5;++k)next.B_[k]=float(2*(d[a[k]]-(b[k]<0?0:d[unsigned(b[k])])));
    for(unsigned k=0;k<5;++k){scorpio_detail::Vector<7> rhs{},w{};rhs[a[k]]=cg[k];if(b[k]>=0)rhs[unsigned(b[k])]=-cg[k];if(!scorpio_detail::linear<7>(g,rhs,w))return false;next.C_[k]=float(w[4]);
        for(unsigned m=0;m<5;++m)next.A_[m][k]=float(2*(w[a[m]]-(b[m]<0?0:w[unsigned(b[m])]))-(m==k?1:0));}
    // Series C10 companion + actual R9/R10, winding DCR and loaded Level pot.
    const float lower=c.level*100000,upper=(1-c.level)*100000,bottom=lower/(1+lower/1e6f),pot=upper+bottom;
    next.rLoad_=1070+pot;next.levelScale_=pot/(1070+pot)*(pot>0?bottom/pot:0);
    next.rSeries_=next.sourceOhms_+15000+next.dt_/100e-6f;
    next.sourceScale_=33000/(33000+next.rSeries_);next.rPrimary_=next.rSeries_*next.sourceScale_+820;
    next.rParallel_=next.rPrimary_*next.rLoad_/(next.rPrimary_+next.rLoad_);
    double sum=0;for(unsigned n=0;n<=48;++n){const double x=double(n)-48;
        next.fir_[n]=float((x==0?.2:std::sin(2*pi*.1*x)/(pi*x))*(.42-.5*std::cos(2*pi*n/96)+.08*std::cos(4*pi*n/96)));sum+=next.fir_[n]*(n==48?1:2);}
    for(auto& v:next.fir_)v=float(v/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
Scorpio::Value Scorpio::value(float x) const noexcept {
    unsigned lo=0,hi=512;for(unsigned n=0;n<10&&hi-lo>1;++n){const unsigned m=(lo+hi)/2;if(x_[m]<=x)lo=m;else hi=m;}lo=std::min(lo,511u);
    const float h=x_[lo+1]-x_[lo],t=(x-x_[lo])/h,a=y_[lo],d=h*slope_[lo];
    const float b=3*(y_[lo+1]-a)-2*d-h*slope_[lo+1],c=2*(a-y_[lo+1])+d+h*slope_[lo+1];
    return {a+t*(d+t*(b+t*c)),area_[lo]+h*t*(a+t*(d/2+t*(b/3+t*c/4)))};
}
float Scorpio::curve(float x) const noexcept {return prepared_&&std::isfinite(x)?value(std::clamp(x,-4.f,4.f)).y:0;}
void Scorpio::reset() noexcept {state_.fill(0);up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;phi_=cap_=previous_=previousArea_=0;invalid_=clamps_=failures_=tableClamps_=0;probe_={};running_=prepared_;}
float Scorpio::sample(float input) noexcept {
    float linear=D_*input;for(unsigned k=0;k<5;++k)linear+=C_[k]*state_[k];
    std::array<float,5> next{};for(unsigned k=0;k<5;++k){next[k]=B_[k]*input;for(unsigned m=0;m<5;++m)next[k]+=A_[k][m]*state_[m];}state_=next;
    float x=linear*inverseGain_;if(std::abs(x)>4){count(tableClamps_);x=std::clamp(x,-4.f,4.f);}
    const auto v=value(x);const float dx=x-previous_;
    const float preamp=adaa_?(std::abs(dx)<1e-5f?value(.5f*(x+previous_)).y:(v.F-previousArea_)/dx):v.y;
    previous_=x;previousArea_=v.F;
    // Passive nonlinear inductor: i=phi/L + phi^3/(L*scale^2). Backward Euler
    // solves one monotone cubic with exactly three Newton updates, no BJT solve/exp.
    constexpr float L=20,scale=.008f;const float e=preamp-cap_,th=e*sourceScale_;
    const float A=1+dt_*rParallel_/L,B=saturating_?dt_*rParallel_/(L*scale*scale):0;
    const float rhs=phi_+dt_*rParallel_*th/rPrimary_;float p=rhs/A;
    for(unsigned n=0;n<3;++n)p-=(A*p+B*p*p*p-rhs)/(A+3*B*p*p);
    phi_=p;const float mag=p/L+(saturating_?p*p*p/(L*scale*scale):0);
    const float core=rParallel_*(th/rPrimary_-mag),primaryCurrent=mag+core/rLoad_,terminal=core+820*primaryCurrent;
    cap_+=dt_*(e-terminal)/(rSeries_*100e-6f);
    probe_={preamp,phi_,mag,core,cap_,core*levelScale_};return probe_.outputVolts/8;
}
float Scorpio::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}
    if(std::abs(input)>.5f){count(clamps_);input=std::clamp(input,-.5f,.5f);}
    float output=0;if(factor_==1)output=sample(input);else {
        upIndex_=upIndex_?upIndex_-1:24;up_[upIndex_]=input;
        for(unsigned phase=0;phase<4;++phase){float x=0;unsigned j=upIndex_;
            for(unsigned tap=phase;tap<97;tap+=4){x+=coefficient(tap)*up_[j];if(++j==25)j=0;}
            downIndex_=downIndex_?downIndex_-1:96;down_[downIndex_]=sample(4*x);
            if(phase==0){j=downIndex_;for(unsigned tap=0;tap<97;++tap){output+=coefficient(tap)*down_[j];if(++j==97)j=0;}}}
    }
    if(!std::isfinite(output)){count(failures_);state_.fill(0);phi_=cap_=previous_=previousArea_=0;return 0;}
    if(std::abs(output)>1){count(failures_);return std::clamp(output,-1.f,1.f);}return output;
}
}

