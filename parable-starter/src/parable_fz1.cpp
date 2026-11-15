#include "parable_fz1.h"
#include "germanium_device.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr std::array<std::array<double,6>,6> incidence={{{1,-1,0,0,0,0},{1,0,0,0,0,0},{0,0,1,0,0,0},{0,0,1,-1,0,0},{0,0,0,0,1,0},{0,0,0,0,1,-1}}};
bool range(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
void count(uint32_t& n,uint32_t add=1) noexcept {n+=std::min(add,std::numeric_limits<uint32_t>::max()-n);}
double dot(const Fz1::Nodes& a,const Fz1::Nodes& b) noexcept {double y=0;for(unsigned n=0;n<6;++n)y+=a[n]*b[n];return y;}
Fz1::Ports ports(const Fz1::Nodes& n) noexcept {Fz1::Ports p{};for(unsigned i=0;i<6;++i)p[i]=dot(incidence[i],n);return p;}
}
bool Fz1::matrices(bool dc) noexcept {
    // Fixed six-node circuit only. Exact linear elimination, adapted owned Muff.
    std::array<Nodes,6> a{},inv{};Nodes rhs{};
    const auto resistor=[&](unsigned node,double ohms,double rail=0.) {a[node][node]+=1/ohms;rhs[node]+=rail/ohms;};
    resistor(B1,1e6);resistor(E1,1e4);resistor(B2,2200+22000*50000*config_.attack/(22000+50000*config_.attack));
    if(!config_.openBiasFeed)resistor(B2,470e3,config_.supplyVolts);
    resistor(C2,1500,config_.supplyVolts);resistor(B3,1e4);resistor(C3,1e4,config_.supplyVolts);
    for(auto& c:caps_) {
        c.g=dc?0:1/(c.series+1/(2*rate_*c.farads));
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j)a[i][j]+=c.g*c.a[i]*c.a[j];
    }
    for(unsigned i=0;i<6;++i)inv[i][i]=1;
    for(unsigned i=0;i<6;++i) {
        unsigned pivot=i;for(unsigned j=i+1;j<6;++j)if(std::abs(a[j][i])>std::abs(a[pivot][i]))pivot=j;
        if(!std::isfinite(a[pivot][i])||std::abs(a[pivot][i])<1e-18)return false;
        std::swap(a[i],a[pivot]);std::swap(inv[i],inv[pivot]);
        const double div=a[i][i];for(unsigned j=0;j<6;++j){a[i][j]/=div;inv[i][j]/=div;}
        for(unsigned row=0;row<6;++row)if(row!=i){const double s=a[row][i];for(unsigned col=0;col<6;++col){a[row][col]-=s*a[i][col];inv[row][col]-=s*inv[i][col];}}
    }
    open_={};current_={};history_={};impedance_={};bcG_=dc?0:2*rate_*100e-12;
    for(unsigned i=0;i<6;++i)for(unsigned n=0;n<6;++n) {
        open_[i]+=inv[i][n]*rhs[n];
        for(unsigned p=0;p<6;++p)current_[i][p]+=inv[i][n]*incidence[p][n];
        for(unsigned c=0;c<4;++c)history_[i][c]+=inv[i][n]*caps_[c].a[n]*caps_[c].g;
    }
    for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j)for(unsigned n=0;n<6;++n)impedance_[i][j]+=incidence[i][n]*current_[n][j];
    // MIT ACME bjt two-port terminal-resistance relation. Tiny RC is never inverted.
    for(unsigned q=0;q<3;++q){const unsigned p=2*q;impedance_[p][p]+=1.885+.3064;impedance_[p][p+1]+=1.885;impedance_[p+1][p]+=1.885;impedance_[p+1][p+1]+=1.885+.000001727;}
    return true;
}
bool Fz1::prepare(float rate,Fz1Config c,unsigned factor) noexcept {
    if((factor!=1&&factor!=2&&factor!=4&&factor!=8)||!range(rate,44100,factor==1?1536000:96000)
       ||!range(c.attack,0,1)||!range(c.volume,0,1)||!range(c.supplyVolts,2.7,3.3)
       ||!range(c.sourceOhms,100,10000)||!range(c.loadOhms,1e5,2e6)||!range(c.leakageScale,.5,1.5))return false;
    Fz1 next;next.config_=c;next.factor_=factor;next.rate_=rate*factor;next.taps_=48*factor+1;next.upSize_=(next.taps_+factor-1)/factor;
    const double bottom=c.volume*500e3,loaded=bottom*c.loadOhms/(bottom+c.loadOhms);
    const double branch=(1-c.volume)*500e3+loaded,terminal=56000*branch/(56000+branch);
    next.jackOhms_=terminal*loaded/branch;
    const auto cap=[&](unsigned i,double farads,double series,int p,int n) {
        auto& part=next.caps_[i];part.farads=farads;part.series=series;
        if(p>=0)part.a[unsigned(p)]=1;
        if(n>=0)part.a[unsigned(n)]=-1;
    };
    cap(0,10e-9,100e3+c.sourceOhms,-1,B1);cap(1,20e-6,0,E1,B2);
    cap(2,20e-6,0,C2,B3);cap(3,3.3e-9,terminal,C3,-1);
    if(!next.matrices(true))return false;
    auto target=ports(next.open_);target[1]-=c.supplyVolts;
    Ports y{.06,-1,.07,-2.5,.05,-1.7},current{};double residual=0;unsigned loops=0;
    if(!next.solve(y,target,120,residual,loops,current))return false;
    next.biasPorts_=y;next.biasCurrents_=current;next.bias_=next.reconstruct(next.open_,current);
    for(unsigned i=0;i<4;++i)next.capBias_[i]=dot(next.caps_[i].a,next.bias_);
    if(!next.matrices(false))return false;
    // Owned Blackman FIR idiom; normal8x385taps, comparisons4x193/2x97.
    // Same48-base-frame span, no interpolation/table replacement of GP.
    double sum=0;
    for(unsigned i=0;i<next.taps_;++i){const double n=double(i)-(next.taps_-1)/2.,fc=.4/factor;const double v=(n==0?2*fc:std::sin(2*pi*fc*n)/(pi*n))*(.42-.5*std::cos(2*pi*i/(next.taps_-1))+.08*std::cos(4*pi*i/(next.taps_-1)));next.fir_[i]=float(v);sum+=v;}
    for(unsigned i=0;i<next.taps_;++i)next.fir_[i]=float(next.fir_[i]/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
bool Fz1::solve(Ports& y,const Ports& target,unsigned budget,double& residual,unsigned& loops,Ports& current) const noexcept {
    GermaniumDevice device;device.bcLeakageAmps*=config_.leakageScale;
    const auto evaluate=[&](const Ports& v,Ports& f,Ports& d,Matrix& jac) noexcept {
        std::array<std::array<double,2>,6> g{};
        for(unsigned q=0;q<3;++q) {
            const unsigned p=2*q;const auto c=germanium_detail::device(v[p],v[p+1],device);if(!c.valid)return std::numeric_limits<double>::infinity();
            d[p]=c.collector+c.base;d[p+1]=-c.collector+bcG_*(v[p+1]-biasPorts_[p+1]-bcHistory_[q]);
            g[p]={c.collectorE+c.baseE,c.collectorC+c.baseC};g[p+1]={-c.collectorE,-c.collectorC+bcG_};
        }
        double norm=0;jac={};
        for(unsigned i=0;i<6;++i) {
            f[i]=v[i]-target[i];for(unsigned k=0;k<6;++k)f[i]+=impedance_[i][k]*(d[k]-biasCurrents_[k]);
            for(unsigned q=0;q<3;++q)for(unsigned j=0;j<2;++j)jac[i][2*q+j]=impedance_[i][2*q]*g[2*q][j]+impedance_[i][2*q+1]*g[2*q+1][j];
            jac[i][i]+=1;if(!std::isfinite(f[i]))return std::numeric_limits<double>::infinity();norm=std::max(norm,std::abs(f[i]));
        }
        return norm;
    };
    const double tolerance=budget==120?1e-12:1e-9;
    Matrix jac{};Ports f{},d{};residual=evaluate(y,f,d,jac);loops=1;
    for(unsigned update=0;update<=budget;++update) {
        if(residual<=tolerance){current=d;return true;}if(update==budget||!std::isfinite(residual))return false;
        for(unsigned i=0;i<6;++i) {
            unsigned pivot=i;for(unsigned r=i+1;r<6;++r)if(std::abs(jac[r][i])>std::abs(jac[pivot][i]))pivot=r;
            if(!std::isfinite(jac[pivot][i])||std::abs(jac[pivot][i])<1e-18)return false;
            std::swap(jac[i],jac[pivot]);std::swap(f[i],f[pivot]);
            for(unsigned r=i+1;r<6;++r){const double scale=jac[r][i]/jac[i][i];for(unsigned k=i+1;k<6;++k)jac[r][k]-=scale*jac[i][k];f[r]-=scale*f[i];}
        }
        Ports delta{};for(int i=5;i>=0;--i){double v=f[unsigned(i)];for(unsigned j=unsigned(i)+1;j<6;++j)v-=jac[unsigned(i)][j]*delta[j];delta[unsigned(i)]=v/jac[unsigned(i)][unsigned(i)];}
        double limiter=1;for(unsigned i=0;i<6;++i){const double v=delta[i];if(!std::isfinite(v))return false;limiter=std::max({limiter,-v/std::max(.05,.05-y[i]),std::abs(v)});}
        // Voltage-limited Newton (owned Muff idiom), bounded domain backtracking.
        // Do not demand monotone infinity-norm residual: this stiff coupled
        // system can require a temporary increase even for a convergent step.
        // Reuse accepted currents/Jacobian; no duplicate normal evaluation.
        bool accepted=false;double scale=1/limiter;
        for(unsigned attempt=0;attempt<8;++attempt,scale*=.5) {
            Ports candidate{};for(unsigned i=0;i<6;++i)candidate[i]=y[i]-scale*delta[i];
            Ports cf{},cd{};Matrix cj{};const double norm=evaluate(candidate,cf,cd,cj);++loops;
            if(std::isfinite(norm)){y=candidate;f=cf;d=cd;jac=cj;residual=norm;accepted=true;break;}
        }
        if(!accepted)return false;
    }
    return false;
}
Fz1::Nodes Fz1::reconstruct(const Nodes& open,const Ports& current) const noexcept {
    Nodes n=open;for(unsigned i=0;i<6;++i)for(unsigned p=0;p<6;++p)n[i]-=current_[i][p]*current[p];return n;
}
Fz1::Nodes Fz1::nodes() const noexcept {Nodes n=nodes_;for(auto& v:n)v=-v;return n;}
Fz1::Nodes Fz1::biasNodes() const noexcept {Nodes n=bias_;for(auto& v:n)v=-v;return n;}
void Fz1::resetCircuit() noexcept {
    previous_=biasPorts_;nodes_=bias_;currents_=biasCurrents_;bcHistory_={};jack_=inputAmps_=residual_=0;
    for(unsigned i=0;i<4;++i){caps_[i].z=0;capVolts_[i]=capBias_[i];}
}
void Fz1::reset() noexcept {
    resetCircuit();up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;
    failures_=inputClamps_=outputClamps_=invalid_=evaluations_=solves_=maximum_=0;running_=prepared_;
}
double Fz1::sample(double physicalInput) noexcept {
    const double source=-physicalInput;Nodes open{};
    for(unsigned i=0;i<6;++i)for(unsigned c=0;c<4;++c)open[i]+=history_[i][c]*(caps_[c].z-(c==0?source:0));
    auto target=ports(open);for(unsigned p=0;p<6;++p)target[p]+=biasPorts_[p];
    Ports y=previous_,current{};double residual=0;unsigned loops=0;bool ok=solve(y,target,kUpdates,residual,loops,current);
    count(solves_);count(evaluations_,loops);maximum_=std::max<uint32_t>(maximum_,loops);
    std::array<double,4> values{},history{};Nodes n{};double output=0,inCurrent=0;
    if(ok) {
        Ports delta{};for(unsigned p=0;p<6;++p)delta[p]=current[p]-biasCurrents_[p];const auto change=reconstruct(open,delta);
        for(unsigned i=0;i<6;++i){n[i]=bias_[i]+change[i];ok=ok&&range(n[i],-12,12);}
        for(unsigned c=0;c<4;++c) {
            const auto& p=caps_[c];const double drive=dot(p.a,change)+(c==0?source:0),i=p.g*(drive-p.z),v=drive-p.series*i;
            values[c]=capBias_[c]+v;history[c]=2*v-p.z;ok=ok&&range(values[c],-24,24)&&range(history[c],-48,48);
            if(c==0)inCurrent=-i;
            if(c==3)output=-i*jackOhms_;
        }
    }
    if(!ok){count(failures_);resetCircuit();residual_=residual;return 0;}
    previous_=y;currents_=current;nodes_=n;capVolts_=values;jack_=output;inputAmps_=inCurrent;residual_=residual;
    for(unsigned c=0;c<4;++c)caps_[c].z=history[c];
    for(unsigned q=0;q<3;++q)bcHistory_[q]=2*(y[2*q+1]-biasPorts_[2*q+1])-bcHistory_[q];
    return output;
}
float Fz1::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){input=0;count(invalid_);}if(std::abs(input)>.5f){input=std::clamp(input,-.5f,.5f);count(inputClamps_);}
    double result=0;
    if(factor_==1)result=sample(input);
    else {
        upIndex_=upIndex_==0?upSize_-1:upIndex_-1;up_[upIndex_]=input;
        for(unsigned phase=0;phase<factor_;++phase) {
            float value=0;auto index=upIndex_;for(unsigned tap=phase;tap<taps_;tap+=factor_){value+=fir_[tap]*up_[index];if(++index==upSize_)index=0;}
            downIndex_=downIndex_==0?taps_-1:downIndex_-1;down_[downIndex_]=float(sample(factor_*value));
            if(phase==0){index=downIndex_;for(unsigned tap=0;tap<taps_;++tap){result+=fir_[tap]*down_[index];if(++index==taps_)index=0;}}
        }
    }
    result/=4; // identical fixed volts/headroom for both states, not hardware calibration
    if(!std::isfinite(result)){count(failures_);resetCircuit();return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}
    return float(result);
}
} // namespace parable::dsp
