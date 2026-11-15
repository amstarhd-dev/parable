#include "parable_blender.h"
#include "blender_device.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr std::array<int,14> plus={Blender::B1,Blender::B1,Blender::C1,Blender::C1,Blender::B3,Blender::B3,Blender::B4,Blender::B4,Blender::B5,Blender::B5,Blender::B4,Blender::B4,-1,Blender::Clip};
constexpr std::array<int,14> minus={Blender::E1,Blender::C1,Blender::E2,-2,Blender::E3,Blender::C3,Blender::E4,Blender::C4,Blender::E5,Blender::C5,Blender::CollectorDiode,Blender::EmitterDiode,Blender::Clip,-1};
bool range(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
void count(uint32_t& v,uint32_t n=1) noexcept {v+=std::min(n,std::numeric_limits<uint32_t>::max()-v);}
}
double Blender::node(const Nodes& v,int n,bool physical) const noexcept {return n>=0?v[std::size_t(n)]:n==Rail&&physical?config_.supplyVolts:0;}
Blender::Ports Blender::ports(const Nodes& v,bool physical) const noexcept {
    Ports y{};for(std::size_t i=0;i<kPorts;++i)y[i]=node(v,plus[i],physical)-node(v,minus[i],physical);return y;
}
bool Blender::matrices(bool dc) noexcept {
    // One literal fixed circuit. Zero-ohm control endpoints are exact unions,
    // including ground, rather than arbitrary small resistor approximations.
    struct R {int p,n;double ohms;};std::array<R,48> resistors{};std::size_t nr=0;
    const auto r=[&](int p,int n,double ohms){resistors[nr++]={p,n,ohms};};
    r(Input,Ground,config_.sourceOhms);r(B1,Ground,150e3);r(B1,Feedback,560e3);
    r(E1,Ground,1500);r(C1,Rail,47e3);r(E2,Feedback,120e3);r(E2,Ground,10e3);
    r(SustainTop,SustainWiper,10e3*(1-config_.sustain));r(SustainWiper,Dry,10e3*config_.sustain);r(Dry,Ground,680);
    r(B3,Ground,150e3);r(B3,Rail,220e3);r(C3,Rail,8200);r(E3,Ground,8200);
    r(CollectorDiode,Ground,27e3);r(EmitterDiode,Ground,27e3);r(B4,Rail,100e3);r(C4,Rail,10e3);r(E4,Ground,820);
    r(ToneIn,ToneLow,27e3);r(ToneLow,ToneDark,3900);
    r(ToneDark,ToneWiper,25e3*config_.tone);r(ToneWiper,ToneBright,25e3*(1-config_.tone));
    r(ToneWiper,LevelTop,100e3);if(config_.boost)r(ToneIn,ToneWiper,47e3);else r(ToneWiper,LevelTop,0);
    r(LevelTop,LevelWiper,50e3*(1-config_.level));r(LevelWiper,Ground,50e3*config_.level);
    r(B5,Ground,18e3);r(B5,Rail,150e3);r(C5,Rail,10e3);r(E5,Ground,820);
    r(Wet,BlendWiper,100e3*(1-config_.blend));r(BlendWiper,Dry,100e3*config_.blend);r(Jack,Ground,config_.loadOhms);
    if(!config_.effect)r(Input,Jack,0);
    std::array<int,kNodes+2> parent{},map{};for(std::size_t i=0;i<parent.size();++i)parent[i]=int(i);
    const auto id=[](int n){return n>=0?n:n==Ground?int(kNodes):int(kNodes+1);};
    const auto root=[&](int n){for(std::size_t i=0;i<parent.size()&&parent[std::size_t(n)]!=n;++i)n=parent[std::size_t(n)];return n;};
    for(std::size_t i=0;i<nr;++i)if(resistors[i].ohms==0)parent[std::size_t(root(id(resistors[i].p)))]=root(id(resistors[i].n));
    std::array<int,kNodes+2> group{};group.fill(-3);group[std::size_t(root(int(kNodes)))]=Ground;group[std::size_t(root(int(kNodes+1)))]=Rail;
    std::size_t dim=0;
    for(std::size_t i=0;i<parent.size();++i){const int t=root(int(i));if(group[std::size_t(t)]==-3)group[std::size_t(t)]=int(dim++);map[i]=group[std::size_t(t)];}
    const auto mapped=[&](int n){return map[std::size_t(id(n))];};
    std::array<Nodes,kNodes> a{},inverse{};Nodes rhs{};std::array<Ports,kNodes> inject{};
    const auto stamp=[&](int p,int n,double g){
        p=mapped(p);n=mapped(n);if(p==n)return;
        if(p>=0){a[std::size_t(p)][std::size_t(p)]+=g;if(n>=0)a[std::size_t(p)][std::size_t(n)]-=g;else if(n==Rail)rhs[std::size_t(p)]+=g*config_.supplyVolts;}
        if(n>=0){a[std::size_t(n)][std::size_t(n)]+=g;if(p>=0)a[std::size_t(n)][std::size_t(p)]-=g;else if(p==Rail)rhs[std::size_t(n)]+=g*config_.supplyVolts;}
    };
    for(std::size_t i=0;i<nr;++i)if(resistors[i].ohms>0)stamp(resistors[i].p,resistors[i].n,1/resistors[i].ohms);
    for(auto& c:caps_){c.g=dc||c.farads==0?0:1/(c.series+1/(1.5*rate_*c.farads));stamp(c.p,c.n,c.g);}
    const auto add=[&](int n,std::size_t port,double value){const int m=mapped(n);if(m>=0)inject[std::size_t(m)][port]+=value;};
    for(std::size_t q=0;q<5;++q){
        const int b=plus[2*q],e=minus[2*q],c=minus[2*q+1];
        add(c,2*q,1);add(b,2*q,1.0/config_.beta);add(e,2*q,-(1+1.0/config_.beta));
        add(c,2*q+1,-1.25);add(b,2*q+1,.25);add(e,2*q+1,1);
    }
    for(std::size_t d=0;d<4;++d)if(!(d==0&&config_.d1Open)){
        // Add/subtract only exponential zero-bias conductance exactly. The
        // physical800k shunt remains in the linear network, not subtracted.
        stamp(plus[10+d],minus[10+d],diodeG_+1/800e3);add(plus[10+d],10+d,1);add(minus[10+d],10+d,-1);
    }
    for(std::size_t i=0;i<dim;++i)inverse[i][i]=1;
    for(std::size_t i=0;i<dim;++i){
        std::size_t pivot=i;for(std::size_t row=i+1;row<dim;++row)if(std::abs(a[row][i])>std::abs(a[pivot][i]))pivot=row;
        if(!std::isfinite(a[pivot][i])||std::abs(a[pivot][i])<1e-18)return false;
        if(pivot!=i){std::swap(a[i],a[pivot]);std::swap(inverse[i],inverse[pivot]);}
        const double div=a[i][i];for(std::size_t j=0;j<dim;++j){a[i][j]/=div;inverse[i][j]/=div;}
        for(std::size_t row=0;row<dim;++row)if(row!=i){const double scale=a[row][i];for(std::size_t col=0;col<dim;++col){a[row][col]-=scale*a[i][col];inverse[row][col]-=scale*inverse[i][col];}}
    }
    open_={};source_={};current_={};history_={};
    for(std::size_t n=0;n<kNodes;++n){
        const int row=mapped(int(n));if(row<0){open_[n]=row==Rail?config_.supplyVolts:0;continue;}
        const auto& inv=inverse[std::size_t(row)];
        for(std::size_t j=0;j<dim;++j){open_[n]+=inv[j]*rhs[j];for(std::size_t p=0;p<kPorts;++p)current_[n][p]+=inv[j]*inject[j][p];}
        source_[n]=inv[std::size_t(mapped(Input))]/config_.sourceOhms;
        for(std::size_t c=0;c<kCaps;++c){const int p=mapped(caps_[c].p),m=mapped(caps_[c].n);history_[n][c]=caps_[c].g*((p>=0?inv[std::size_t(p)]:0)-(m>=0?inv[std::size_t(m)]:0));}
    }
    for(std::size_t i=0;i<kPorts;++i)for(std::size_t j=0;j<kPorts;++j)impedance_[i][j]=(plus[i]>=0?current_[std::size_t(plus[i])][j]:0)-(minus[i]>=0?current_[std::size_t(minus[i])][j]:0);
    return true;
}
bool Blender::prepare(float rate,BlenderConfig c,BlenderRate mode) noexcept {
    const unsigned factor=unsigned(mode);
    if((factor!=1&&factor!=2&&factor!=4&&factor!=8)||!range(rate,44100,factor==1?3072000:96000)
       ||!range(c.sustain,0,1)||!range(c.tone,0,1)||!range(c.blend,0,1)||!range(c.level,0,1)
       ||!range(c.supplyVolts,7.5,10)||!range(c.temperatureC,5,40)||!range(c.beta,250,500)
       ||!range(c.sourceOhms,100,10000)||!range(c.loadOhms,100000,2000000)||!range(c.junctionScale,.5,2))return false;
    Blender next;next.config_=c;next.factor_=factor;next.taps_=48*factor+1;next.rate_=double(rate)*factor;
    const double t=273.15+double(c.temperatureC),ratio=t/300,k=blender_detail::kOverQ;
    next.vt_=k*t;next.si_=1e-14*std::pow(ratio,3)*std::exp(1.11/k*(1/300.-1/t));
    next.ge_=560e-9*std::pow(ratio,3/blender_detail::diodeN)*std::exp(.67/(k*blender_detail::diodeN)*(1/300.-1/t));
    next.theta_=1/(blender_detail::diodeN*next.vt_);next.diodeG_=next.ge_*next.theta_;
    const auto cap=[&](std::size_t i,double f,int p,int n,double r=0){auto& part=next.caps_[i];part.p=p;part.n=n;part.farads=f;part.series=r;};
    cap(0,100e-9,Input,B1,15e3);cap(1,50e-9,Feedback,Ground);cap(2,10e-6,E2,SustainTop);cap(3,100e-9,SustainWiper,B3);
    cap(4,10e-6,C3,CollectorDiode);cap(5,10e-6,E3,EmitterDiode);cap(6,10e-6,E4,Ground);cap(7,10e-6,C4,Clip);
    cap(8,10e-6,Clip,ToneIn);cap(9,100e-9,ToneLow,Ground);cap(10,2.7e-9,ToneIn,ToneBright);
    cap(11,10e-6,LevelWiper,B5,10e3);cap(12,1e-9,B5,C5,3900);cap(13,10e-6,E5,Ground);cap(14,10e-6,C5,Wet);
    if(c.effect)cap(15,100e-9,BlendWiper,Jack); // off: original SPDT leaves this end open.
    for(std::size_t q=0;q<5;++q){cap(16+2*q,4e-12*c.junctionScale,plus[2*q],minus[2*q+1]);cap(17+2*q,20e-12*c.junctionScale,plus[2*q],minus[2*q]);}
    for(std::size_t d=0;d<4;++d)if(!(d==0&&c.d1Open))cap(26+d,.5e-12/std::pow(.6,.29)*c.junctionScale,plus[10+d],minus[10+d]);
    if(!next.matrices(true))return false;
    Ports y{.59,-4,.63,-4.2,.63,-2.5,.64,c.d1Open?.6:-1.5,.63,-4.2,.13,.13,0,0};Evaluation e;uint32_t loops=0;double residual=0;
    if(!next.solve(y,next.ports(next.open_,true),true,residual,loops,e))return false;
    next.biasPorts_=y;next.biasCurrents_=e.i;next.biasRaw_=e.raw;next.biasCharge_=e.q;next.bias_=next.reconstruct(next.open_,e.i);
    for(std::size_t i=0;i<kCaps;++i)next.capBias_[i]=next.node(next.bias_,next.caps_[i].p,true)-next.node(next.bias_,next.caps_[i].n,true);
    if(!next.matrices(false))return false;
    double sum=0;
    for(unsigned i=0;i<next.taps_;++i){const double n=double(i)-24*factor,fc=.4/factor;const double h=(n==0?2*fc:std::sin(2*pi*fc*n)/(pi*n))*(.42-.5*std::cos(2*pi*i/(next.taps_-1))+.08*std::cos(4*pi*i/(next.taps_-1)));next.fir_[i]=float(h);sum+=next.fir_[i];}
    for(unsigned i=0;i<next.taps_;++i)next.fir_[i]=float(next.fir_[i]/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
bool Blender::evaluate(const Ports& v,bool dc,Evaluation& out) const noexcept {
    out={};const double h=dc?0:1.5*rate_;
    for(std::size_t i=0;i<kPorts;++i){
        if(i==10&&config_.d1Open){if(!range(v[i],-32,32))return false;continue;}
        const bool diode=i>=10;const auto d=blender_detail::junction(v[i],diode?theta_:1/vt_,diode?ge_:si_,diode?blender_detail::transit:0);
        if(!d.valid)return false;
        out.raw[i]=d.i;out.q[i]=d.q;out.i[i]=d.i;out.g[i]=d.g;
        if(diode){out.i[i]-=diodeG_*v[i];out.g[i]-=diodeG_;out.i[i]+=h*(d.q-biasCharge_[i]-chargeZ_[i-10]);out.g[i]+=h*d.c;}
    }return true;
}
bool Blender::solve(Ports& y,const Ports& target,bool dc,double& residual,uint32_t& loops,Evaluation& last) const noexcept {
    const uint32_t budget=dc?160:kUpdates;
    for(loops=0;loops<=budget;++loops){
        Evaluation d;if(!evaluate(y,dc,d))return false;
        Matrix j{};Ports f{};residual=0;
        for(std::size_t i=0;i<kPorts;++i){f[i]=y[i]-target[i];for(std::size_t k=0;k<kPorts;++k){f[i]+=impedance_[i][k]*(d.i[k]-biasCurrents_[k]);j[i][k]=impedance_[i][k]*d.g[k];}j[i][i]+=1;residual=std::max(residual,std::abs(f[i]));if(!std::isfinite(f[i]))return false;}
        if(residual<=(dc?1e-11:1e-9)){last=d;return true;}if(loops==budget)return false;
        for(std::size_t i=0;i<kPorts;++i){
            std::size_t pivot=i;for(std::size_t r=i+1;r<kPorts;++r)if(std::abs(j[r][i])>std::abs(j[pivot][i]))pivot=r;
            if(!std::isfinite(j[pivot][i])||std::abs(j[pivot][i])<1e-18)return false;
            if(pivot!=i){std::swap(j[i],j[pivot]);std::swap(f[i],f[pivot]);}
            for(std::size_t r=i+1;r<kPorts;++r){const double scale=j[r][i]/j[i][i];for(std::size_t c=i+1;c<kPorts;++c)j[r][c]-=scale*j[i][c];f[r]-=scale*f[i];}
        }
        Ports delta{};for(int i=int(kPorts)-1;i>=0;--i){double x=f[std::size_t(i)];for(std::size_t k=std::size_t(i)+1;k<kPorts;++k)x-=j[std::size_t(i)][k]*delta[k];delta[std::size_t(i)]=x/j[std::size_t(i)][std::size_t(i)];}
        double scale=1;for(std::size_t i=0;i<kPorts;++i){if(!std::isfinite(delta[i]))return false;if(!(i==10&&config_.d1Open))scale=std::max(scale,-delta[i]/std::max(.05,.05-y[i]));scale=std::max(scale,std::abs(delta[i])/4);}
        for(std::size_t i=0;i<kPorts;++i)y[i]-=delta[i]/scale;
    }return false;
}
Blender::Nodes Blender::reconstruct(const Nodes& open,const Ports& currents) const noexcept {
    Nodes out=open;for(std::size_t i=0;i<kNodes;++i)for(std::size_t p=0;p<kPorts;++p)out[i]-=current_[i][p]*currents[p];return out;
}
void Blender::observe(const Ports& y,const Ports& raw,const Ports& charge) noexcept {
    for(std::size_t q=0;q<5;++q){const double f=raw[2*q],r=raw[2*q+1],base=f/config_.beta+r/4,col=f-1.25*r;transistor_[q]={base,col,-base-col};}
    for(std::size_t d=0;d<4;++d){
        if(d==0&&config_.d1Open){diodes_[d]=branches_[d]=0;continue;}
        diodes_[d]=raw[10+d]+1.5*rate_*(charge[10+d]-biasCharge_[10+d]-chargeZ_[d]);
        branches_[d]=diodes_[d]+y[10+d]/800e3+capAmps_[26+d];
    }
}
void Blender::resetCircuit() noexcept {
    previous_=biasPorts_;nodes_=bias_;capVolts_=capBias_;capAmps_={};chargeZ_={};chargePrevious_={};
    for(auto& c:caps_){c.z=0;c.previous=0;}jack_=inputAmps_=residual_=0;observe(biasPorts_,biasRaw_,biasCharge_);
}
void Blender::reset() noexcept {
    resetCircuit();up_={};down_={};upIndex_=downIndex_=failures_=invalid_=inputClamps_=outputClamps_=evaluations_=solves_=maximum_=0;running_=prepared_;
}
double Blender::sample(double input) noexcept {
    Nodes open{};for(std::size_t n=0;n<kNodes;++n){open[n]=source_[n]*input;for(std::size_t c=0;c<kCaps;++c)open[n]+=history_[n][c]*caps_[c].z;}
    auto target=ports(open);for(std::size_t p=0;p<kPorts;++p)target[p]+=biasPorts_[p];
    Ports y=previous_;Evaluation d;double residual=0;uint32_t loops=0;bool ok=solve(y,target,false,residual,loops,d);
    count(solves_);count(evaluations_,loops+1);maximum_=std::max(maximum_,loops+1);
    Nodes n{},change{};std::array<double,kCaps> volts{},amps{},history{},previous{};
    if(ok){
        Ports delta{};for(std::size_t p=0;p<kPorts;++p)delta[p]=d.i[p]-biasCurrents_[p];change=reconstruct(open,delta);
        for(std::size_t k=0;k<kNodes;++k){n[k]=bias_[k]+change[k];ok=ok&&range(n[k],-32,32);}
        for(std::size_t c=0;c<kCaps;++c){const auto& p=caps_[c];const double drive=node(change,p.p)-node(change,p.n),current=p.g*(drive-p.z),v=drive-p.series*current;volts[c]=capBias_[c]+v;amps[c]=current;history[c]=(4*v-p.previous)/3;previous[c]=v;ok=ok&&range(volts[c],-64,64)&&range(history[c],-64,64);}
    }
    if(!ok){count(failures_);resetCircuit();residual_=residual;return 0;}
    nodes_=n;previous_=y;capVolts_=volts;capAmps_=amps;residual_=residual;jack_=n[Jack]-bias_[Jack];inputAmps_=(input-n[Input])/config_.sourceOhms;
    observe(y,d.raw,d.q);
    for(std::size_t c=0;c<kCaps;++c){caps_[c].z=history[c];caps_[c].previous=previous[c];}
    for(std::size_t j=0;j<4;++j){const double q=d.q[10+j]-biasCharge_[10+j];chargeZ_[j]=(4*q-chargePrevious_[j])/3;chargePrevious_[j]=q;}
    return jack_;
}
float Blender::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){input=0;count(invalid_);}if(std::abs(input)>1){input=std::clamp(input,-1.f,1.f);count(inputClamps_);}
    double result=0;
    if(factor_==1)result=sample(input);
    else{
        upIndex_=upIndex_==0?uint32_t(up_.size()-1):upIndex_-1;up_[upIndex_]=input;
        for(unsigned phase=0;phase<factor_;++phase){
            float interpolated=0;auto index=upIndex_;for(unsigned tap=phase;tap<taps_;tap+=factor_){interpolated+=fir_[tap]*up_[index];if(++index==up_.size())index=0;}
            downIndex_=downIndex_==0?taps_-1:downIndex_-1;down_[downIndex_]=float(sample(factor_*interpolated));
            if(phase==0){index=downIndex_;for(unsigned tap=0;tap<taps_;++tap){result+=fir_[tap]*down_[index];if(++index==taps_)index=0;}}
        }
    }
    result/=8; // declared common software headroom, not codec calibration.
    if(!std::isfinite(result)){count(failures_);resetCircuit();return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}return float(result);
}
} // namespace parable::dsp
