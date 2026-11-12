#include "parable_muff.h"
#include "muff_device.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr std::array<std::size_t,10> positive={Muff::B4,Muff::B4,Muff::B3,Muff::B3,Muff::B2,Muff::B2,Muff::B1,Muff::B1,Muff::D3,Muff::D2};
constexpr std::array<std::size_t,10> negative={Muff::E4,Muff::C4,Muff::E3,Muff::C3,Muff::E2,Muff::C2,Muff::E1,Muff::C1,Muff::C3,Muff::C2};
bool range(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
void count(uint32_t& n,uint32_t add=1) noexcept {n+=std::min(add,std::numeric_limits<uint32_t>::max()-n);}
double dot(const Muff::Nodes& a,const Muff::Nodes& b) noexcept {double y=0;for(std::size_t n=0;n<Muff::kNodes;++n)y+=a[n]*b[n];return y;}
Muff::Ports ports(const Muff::Nodes& n) noexcept {Muff::Ports out{};for(std::size_t p=0;p<10;++p)out[p]=n[positive[p]]-n[negative[p]];return out;}
}
bool Muff::matrices(bool dc) noexcept {
    // One fixed circuit, no runtime netlist. Exact passive/linear elimination.
    std::array<Nodes,kNodes> a{},inverse{};
    Nodes rhs{};std::array<Ports,kNodes> inject{};
    const auto resistor=[&](std::size_t p,int n,double r) {
        const double g=1/r;a[p][p]+=g;
        if(n>=0){a[std::size_t(n)][std::size_t(n)]+=g;a[p][std::size_t(n)]-=g;a[std::size_t(n)][p]-=g;}
    };
    const auto rail=[&](std::size_t node,double r){resistor(node,-1,r);rhs[node]+=9/r;};
    for(std::size_t q=0;q<4;++q) {
        const std::size_t b=3*q,c=b+1,e=b+2;
        rail(c,q==3?15e3:10e3);resistor(e,-1,q==0?100:q==3?3300:150);
        resistor(b,-1,q==0?47e3:100e3);
        if(q<3)resistor(b,int(c),q==2&&config_.highR15?2.2e6:470e3);
        else rail(b,430e3);
        // Forward/reverse transport-current injections of the same NPN device.
        inject[c][2*q]=1;inject[b][2*q]=1/muff_detail::bf;inject[e][2*q]=-(1+1/muff_detail::bf);
        inject[c][2*q+1]=-(1+1/muff_detail::br);inject[b][2*q+1]=1/muff_detail::br;inject[e][2*q+1]=1;
    }
    resistor(Sustain,-1,1e3+1e5*config_.sustain);
    resistor(C2,ToneLow,39e3);resistor(ToneHigh,-1,22e3);resistor(ToneLow,ToneHigh,100e3);
    for(std::size_t k=8;k<10;++k) {
        resistor(positive[k],int(negative[k]),1/muff_detail::diodeG0);
        inject[positive[k]][k]=1;inject[negative[k]][k]=-1;
    }
    for(auto& c:caps_) {
        c.g=dc?0:1/(c.series+1/(2*rate_*c.farads));
        for(std::size_t i=0;i<kNodes;++i)for(std::size_t j=0;j<kNodes;++j)a[i][j]+=c.g*c.a[i]*c.a[j];
    }
    // Stopped-only bounded17x17 inverse, partial pivoting (owned Ge solver idiom).
    for(std::size_t i=0;i<kNodes;++i)inverse[i][i]=1;
    for(std::size_t i=0;i<kNodes;++i) {
        std::size_t pivot=i;for(std::size_t j=i+1;j<kNodes;++j)if(std::abs(a[j][i])>std::abs(a[pivot][i]))pivot=j;
        if(!std::isfinite(a[pivot][i])||std::abs(a[pivot][i])<1e-18)return false;
        if(pivot!=i){std::swap(a[i],a[pivot]);std::swap(inverse[i],inverse[pivot]);}
        const double div=a[i][i];for(std::size_t j=0;j<kNodes;++j){a[i][j]/=div;inverse[i][j]/=div;}
        for(std::size_t row=0;row<kNodes;++row)if(row!=i) {
            const double scale=a[row][i];
            for(std::size_t col=0;col<kNodes;++col){a[row][col]-=scale*a[i][col];inverse[row][col]-=scale*inverse[i][col];}
        }
    }
    open_={};current_={};history_={};
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t n=0;n<kNodes;++n) {
        open_[i]+=inverse[i][n]*rhs[n];
        for(std::size_t p=0;p<kPorts;++p)current_[i][p]+=inverse[i][n]*inject[n][p];
        for(std::size_t c=0;c<kCaps;++c)history_[i][c]+=inverse[i][n]*caps_[c].a[n]*caps_[c].g;
    }
    for(std::size_t i=0;i<kPorts;++i)for(std::size_t j=0;j<kPorts;++j)impedance_[i][j]=current_[positive[i]][j]-current_[negative[i]][j];
    return true;
}
bool Muff::prepare(float rate,MuffConfig c,MuffRate mode) noexcept {
    if(!range(rate,32000,mode==MuffRate::Reference1x?384000:96000)
       ||!range(c.sustain,0,1)||!range(c.tone,0,1)||!range(c.volume,0,1)
       ||(mode!=MuffRate::Reference1x&&mode!=MuffRate::Oversampled4x))return false;
    Muff next;next.config_=c;next.mode_=mode;next.rate_=rate*(mode==MuffRate::Oversampled4x?4:1);
    const double sustain=c.sustain,tone=c.tone,volume=c.volume;
    const double rb=1e5*volume;next.bottom_=rb*1e6/(rb+1e6);
    const auto cap=[&](std::size_t index,double farads,double series,int p,int n) {
        auto& part=next.caps_[index];part.farads=farads;part.series=series;
        if(p>=0)part.a[std::size_t(p)]=1;
        if(n>=0)part.a[std::size_t(n)]=-1;
    };
    cap(0,1e-6,40e3,-1,B4); // C1 + R2 + declared1k source.
    cap(1,100e-9,1e5*(1-volume)+next.bottom_,C1,-1);
    cap(2,100e-9,1e5*tone*(1-tone),ToneLow,B1);
    next.caps_[2].a[ToneLow]=1-tone;next.caps_[2].a[ToneHigh]=tone; // exact loaded pot Kron reduction.
    cap(3,1e-6,1e5*(1-sustain),C4,Sustain);
    cap(4,100e-9,10e3,Sustain,B3);cap(5,1e-6,0,B3,D3);cap(6,1e-6,0,B2,D2);
    cap(7,10e-9,0,ToneLow,-1);cap(8,4e-9,0,C2,ToneHigh);
    cap(9,470e-12,0,B4,C4);cap(10,470e-12,0,B2,C2);cap(11,470e-12,0,B3,C3);
    cap(12,100e-9,10e3,C3,B2);
    if(!next.matrices(true))return false;
    Ports y{.6,-6.5,.65,-4,.6,-7,.6,-3,0,0},current{};double residual=0;uint32_t loops=0;
    if(!next.solve(y,ports(next.open_),120,residual,loops,current))return false;
    next.biasPorts_=y;next.bias_=next.reconstruct(next.open_,current);next.biasCurrents_=current;
    for(std::size_t k=0;k<kCaps;++k)next.capBias_[k]=dot(next.caps_[k].a,next.bias_);
    if(!next.matrices(false))return false;
    // Owned193-tap4x interpolation/decimation idiom; protected kernels untouched.
    double sum=0;
    for(std::size_t i=0;i<kTaps;++i) {
        const double n=double(i)-(kTaps-1)/2.;
        const double f=(n==0?.2:std::sin(2*pi*.1*n)/(pi*n))*(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1)));
        next.fir_[i]=float(f);sum+=next.fir_[i];
    }
    for(auto& f:next.fir_)f=float(f/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
bool Muff::solve(Ports& y,const Ports& target,uint32_t budget,double& residual,uint32_t& loops,Ports& current) const noexcept {
    for(loops=0;loops<=budget;++loops) {
        const auto d=muff_detail::currents(y);if(!d.valid)return false;
        Matrix j{};Ports f{};residual=0;
        for(std::size_t i=0;i<kPorts;++i) {
            f[i]=y[i]-target[i];
            for(std::size_t k=0;k<kPorts;++k){f[i]+=impedance_[i][k]*(d.i[k]-biasCurrents_[k]);j[i][k]=impedance_[i][k]*d.g[k];}
            j[i][i]+=1;residual=std::max(residual,std::abs(f[i]));
            if(!std::isfinite(f[i]))return false;
        }
        if(residual<=(budget==120?1e-12:1e-9)){current=d.i;return true;}
        if(loops==budget)return false;
        // Fixed10x10 pivoted elimination, adapted from owned four-node Ge solve.
        for(std::size_t i=0;i<kPorts;++i) {
            std::size_t pivot=i;for(std::size_t row=i+1;row<kPorts;++row)if(std::abs(j[row][i])>std::abs(j[pivot][i]))pivot=row;
            if(!std::isfinite(j[pivot][i])||std::abs(j[pivot][i])<1e-18)return false;
            if(pivot!=i){std::swap(j[i],j[pivot]);std::swap(f[i],f[pivot]);}
            for(std::size_t row=i+1;row<kPorts;++row) {
                const double scale=j[row][i]/j[i][i];
                for(std::size_t col=i+1;col<kPorts;++col)j[row][col]-=scale*j[i][col];
                f[row]-=scale*f[i];
            }
        }
        Ports delta{};
        for(int i=int(kPorts)-1;i>=0;--i){double v=f[std::size_t(i)];for(std::size_t k=std::size_t(i)+1;k<kPorts;++k)v-=j[std::size_t(i)][k]*delta[k];delta[std::size_t(i)]=v/j[std::size_t(i)][std::size_t(i)];}
        double scale=1;
        for(std::size_t i=0;i<kPorts;++i) {
            if(!std::isfinite(delta[i]))return false;
            scale=std::max(scale,-delta[i]/std::max(.05,.05-y[i]));
            if(i>=8)scale=std::max(scale,delta[i]/std::max(.05,.05+y[i]));
            scale=std::max(scale,std::abs(delta[i])/2);
        }
        for(std::size_t i=0;i<kPorts;++i)y[i]-=delta[i]/scale;
    }
    return false;
}
Muff::Nodes Muff::reconstruct(const Nodes& open,const Ports& current) const noexcept {
    Nodes n=open;
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t p=0;p<kPorts;++p)n[i]-=current_[i][p]*current[p];
    return n;
}
void Muff::resetCircuit() noexcept {
    previous_=biasPorts_;nodes_=bias_;diodes_={};jack_=tone_=inputAmps_=residual_=0;
    for(std::size_t i=0;i<kCaps;++i){capVolts_[i]=capBias_[i];caps_[i].z=0;}
}
void Muff::reset() noexcept {
    resetCircuit();up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;
    failures_=outputClamps_=inputClamps_=invalid_=evaluations_=solves_=maximum_=0;running_=prepared_;
}
double Muff::sample(double input) noexcept {
    // Exact translation about the solved DC operating point, not a gate or
    // linearization. Avoid subtracting large DC capacitor terms each sample.
    Nodes open{};
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t c=0;c<kCaps;++c)open[i]+=history_[i][c]*(caps_[c].z-(c==0?input:0));
    Ports y=previous_,current{};double residual=0;uint32_t loops=0;
    auto target=ports(open);for(std::size_t p=0;p<kPorts;++p)target[p]+=biasPorts_[p];
    bool ok=solve(y,target,kUpdates,residual,loops,current);
    count(solves_);count(evaluations_,loops+1);maximum_=std::max(maximum_,loops+1);
    Nodes n{};std::array<double,kCaps> values{},history{};
    double output=0,sourceCurrent=0,toneCurrent=0;
    if(ok) {
        Ports difference{};for(std::size_t p=0;p<kPorts;++p)difference[p]=current[p]-biasCurrents_[p];
        const Nodes change=reconstruct(open,difference);
        for(std::size_t i=0;i<kNodes;++i)n[i]=bias_[i]+change[i];
        for(double v:n)ok=ok&&range(v,-16,16); // explicit numerical domain, not an analog rail model.
        for(std::size_t c=0;c<kCaps;++c) {
            const auto& part=caps_[c];const double drive=dot(part.a,change)+(c==0?input:0);
            const double current=part.g*(drive-part.z);
            const double delta=drive-part.series*current;values[c]=capBias_[c]+delta;history[c]=2*delta-part.z;
            ok=ok&&range(values[c],-32,32)&&range(history[c],-64,64);
            if(c==0)sourceCurrent=current;
            if(c==1)output=current*bottom_;
            if(c==2)toneCurrent=current;
        }
    }
    if(!ok){count(failures_);resetCircuit();residual_=residual;return 0;}
    previous_=y;nodes_=n;capVolts_=values;residual_=residual;jack_=output;inputAmps_=sourceCurrent;
    tone_=(1-double(config_.tone))*n[ToneLow]+config_.tone*n[ToneHigh]-caps_[2].series*toneCurrent;
    for(std::size_t i=0;i<2;++i)diodes_[i]=current[8+i]+muff_detail::diodeG0*y[8+i];
    for(std::size_t c=0;c<kCaps;++c)caps_[c].z=history[c];
    return output;
}
float Muff::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){input=0;count(invalid_);}
    if(std::abs(input)>1){input=std::clamp(input,-1.f,1.f);count(inputClamps_);}
    double result=0;
    if(mode_==MuffRate::Reference1x)result=sample(input);
    else {
        upIndex_=upIndex_==0?uint32_t(up_.size()-1):upIndex_-1;up_[upIndex_]=input;
        for(std::size_t phase=0;phase<4;++phase) {
            float interpolated=0;auto index=upIndex_;
            for(std::size_t tap=phase;tap<kTaps;tap+=4){interpolated+=fir_[tap]*up_[index];if(++index==up_.size())index=0;}
            downIndex_=downIndex_==0?kTaps-1:downIndex_-1;
            down_[downIndex_]=float(sample(4*interpolated));
            if(phase==0){index=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){result+=fir_[tap]*down_[index];if(++index==kTaps)index=0;}}
        }
    }
    result/=9; // fixed declared software headroom, not measured ADC/DAC calibration.
    if(!std::isfinite(result)){count(failures_);resetCircuit();return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}
    return float(result);
}
} // namespace parable::dsp
