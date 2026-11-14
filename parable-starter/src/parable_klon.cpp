#include "parable_klon.h"
#include "klon_device.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr std::array<std::size_t,4> positive={Klon::Input,Klon::DrivePlus,Klon::Vref,Klon::Vref};
constexpr std::array<std::size_t,4> negative={Klon::Buffer,Klon::DriveMinus,Klon::SumMinus,Klon::ToneMinus};
constexpr std::array<std::size_t,4> output={Klon::Buffer,Klon::DriveOut,Klon::SumOut,Klon::ToneOut};
bool range(double x,double a,double b) noexcept {return std::isfinite(x)&&x>=a&&x<=b;}
void count(uint32_t& x,uint32_t add=1) noexcept {x+=std::min(add,std::numeric_limits<uint32_t>::max()-x);}
double dot(const Klon::Nodes& a,const Klon::Nodes& b) noexcept {double v=0;for(std::size_t i=0;i<a.size();++i)v+=a[i]*b[i];return v;}
}
bool Klon::matrices(bool dc) noexcept {
    // Specialized fixed S698 network; stopped-only exact elimination.
    std::array<Nodes,kNodes> a{},inverse{};Nodes rhs{};
    const auto resistor=[&](std::size_t p,int n,double r,double rail=0) {
        const double g=1/r;a[p][p]+=g;
        if(n>=0){a[std::size_t(n)][std::size_t(n)]+=g;a[p][std::size_t(n)]-=g;a[std::size_t(n)][p]-=g;}
        else rhs[p]+=g*rail;
    };
    resistor(Input,Vref,1e6);
    for(std::size_t k=0;k<4;++k){resistor(positive[k],negative[k],1e12);resistor(output[k],-1,50,4.5);}
    resistor(Vref,-1,27e3,9);resistor(Vref,-1,27e3);
    resistor(BypassHP,-1,100e3);resistor(BypassHP,Bypass,560);resistor(Bypass,Jack,68e3);
    resistor(LevelTop,Jack,68e3);resistor(Jack,-1,100e3);resistor(Jack,-1,1e6);
    resistor(Pre,CleanUpper,5100);resistor(Pre,DrivePlus,10000);
    resistor(Pre,FF1,1500);resistor(FF1,SumMinus,15000);resistor(CleanUpper,Vref,1500);
    const double g=controls_.gain;
    if(g>0)resistor(DrivePlus,Vref,100e3*g);
    resistor(Feedback,Vref,2000+100e3*(1-g));resistor(Feedback,DriveMinus,15000);
    resistor(DriveMinus,DriveOut,422e3);resistor(Clip,-1,1/klon_detail::g0);
    resistor(PostClip,SumMinus,47000);resistor(SumMinus,CleanWiper,27000);
    resistor(SumMinus,SumOut,392e3);resistor(SumOut,ToneMinus,100e3);resistor(ToneMinus,ToneOut,100e3);
    resistor(SumOut,ToneOut,16500); // P2 track+R21+R23 exact Thevenin reduction
    resistor(CleanUpper,Vref,100e3);resistor(LevelTop,-1,10000);
    for(auto& c:caps_) {
        c.g=dc?0:1/(c.series+1/(2*rate_*c.farads));
        for(std::size_t i=0;i<kNodes;++i)for(std::size_t j=0;j<kNodes;++j)a[i][j]+=c.g*c.a[i]*c.a[j];
    }
    for(std::size_t i=0;i<kNodes;++i)inverse[i][i]=1;
    // Owned main partial-pivot prepared inverse; no callback matrix preparation.
    for(std::size_t i=0;i<kNodes;++i) {
        std::size_t pivot=i;
        for(std::size_t j=i+1;j<kNodes;++j)if(std::abs(a[j][i])>std::abs(a[pivot][i]))pivot=j;
        if(!std::isfinite(a[pivot][i])||std::abs(a[pivot][i])<1e-18)return false;
        if(pivot!=i){std::swap(a[i],a[pivot]);std::swap(inverse[i],inverse[pivot]);}
        const double div=a[i][i];for(std::size_t j=0;j<kNodes;++j){a[i][j]/=div;inverse[i][j]/=div;}
        for(std::size_t row=0;row<kNodes;++row)if(row!=i) {
            const double scale=a[row][i];
            for(std::size_t col=0;col<kNodes;++col){a[row][col]-=scale*a[i][col];inverse[row][col]-=scale*inverse[i][col];}
        }
    }
    // Exact rank-one Schur updates. r=0 imposes a true electrical wire; no
    // arbitrary small-resistance endpoint. P1A contact is the ONLY changed part.
    const auto constrain=[&](const Nodes& v,double r) {
        Nodes column{},row{};
        for(std::size_t i=0;i<kNodes;++i)for(std::size_t j=0;j<kNodes;++j){column[i]+=inverse[i][j]*v[j];row[i]+=v[j]*inverse[j][i];}
        const double denominator=r+dot(v,column);
        if(!std::isfinite(denominator)||denominator<=0)return false;
        for(std::size_t i=0;i<kNodes;++i)for(std::size_t j=0;j<kNodes;++j)inverse[i][j]-=column[i]*row[j]/denominator;
        return true;
    };
    Nodes v{};
    if(g==0){v[DrivePlus]=1;v[Vref]=-1;if(!constrain(v,0))return false;}
    v={};v[CleanUpper]=1-g;v[Vref]=g;v[CleanWiper]=-1;
    if(!constrain(v,100e3*g*(1-g)+(controls_.faultyContact?100e3:0)))return false;
    v={};v[LevelTop]=controls_.level;v[Jack]=-1;
    if(!constrain(v,10000*controls_.level*(1-controls_.level)))return false;
    nodes_={};current_={};history_={};drive_={};
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t j=0;j<kNodes;++j) {
        nodes_[i]+=inverse[i][j]*rhs[j];
        for(std::size_t c=0;c<kCaps;++c)history_[i][c]+=inverse[i][j]*caps_[c].a[j]*caps_[c].g;
    }
    for(std::size_t i=0;i<kNodes;++i){current_[i]=inverse[i][Clip];for(std::size_t k=0;k<4;++k)drive_[i][k]=inverse[i][output[k]]/50;}
    return true;
}
Klon::Nodes Klon::reconstruct(const Nodes& base,const Unknowns& y,double current,bool dc) const noexcept {
    Nodes n=base;
    for(std::size_t i=0;i<kNodes;++i) {
        n[i]-=current_[i]*(current-(dc?0:biasCurrent_));
        for(std::size_t k=0;k<4;++k)n[i]+=drive_[i][k]*(y[1+k]-(dc?0:biasUnknowns_[1+k]));
    }
    return n;
}
bool Klon::evaluate(const Unknowns& y,const Nodes& base,bool dc,Unknowns& f,Matrix& j,double& current,std::array<double,2>& diodes) const noexcept {
    const auto d=klon_detail::diode(y[0]);if(!d.valid)return false;
    for(std::size_t k=1;k<5;++k)if(!range(y[k],-16,16))return false;
    const auto n=reconstruct(base,y,d.i,dc);
    f={};j={};f[0]=y[0]-n[Clip];j[0][0]=1+current_[Clip]*d.g;
    for(std::size_t k=0;k<4;++k)j[0][1+k]=-drive_[Clip][k];
    const double step=2/(3*rate_);
    for(std::size_t k=0;k<4;++k) {
        const double self=drive_[positive[k]][k]-drive_[negative[k]][k];
        const double error=n[positive[k]]-n[negative[k]]-self*y[1+k];
        const double previous=(4*previous_[1+k]-oldU_[k])/3;
        const auto r=klon_detail::response(error,self,previous,step,dc,k);
        f[1+k]=y[1+k]-r.value;j[1+k][1+k]=1;
        j[1+k][0]=r.slope*(current_[positive[k]]-current_[negative[k]])*d.g;
        for(std::size_t other=0;other<4;++other)if(other!=k)j[1+k][1+other]=-r.slope*(drive_[positive[k]][other]-drive_[negative[k]][other]);
    }
    for(double x:f)if(!std::isfinite(x))return false;
    current=d.i;diodes={d.d2,d.d3};return true;
}
bool Klon::solve(Unknowns& y,const Nodes& base,bool dc,double& residual,uint32_t& evaluations,double& current,std::array<double,2>& diodes) const noexcept {
    const uint32_t budget=dc?120:kUpdates;const double tolerance=dc?1e-12:1e-9;
    Unknowns f{};Matrix j{};evaluations=1;if(!evaluate(y,base,dc,f,j,current,diodes))return false;
    const auto norm=[](const Unknowns& v){double sum=0;for(double x:v)sum+=x*x;return sum;};
    for(uint32_t update=0;update<=budget;++update) {
        residual=0;for(double v:f)residual=std::max(residual,std::abs(v));
        if(residual<tolerance)return true;
        if(update==budget)return false;
        const double oldNorm=norm(f);
        for(std::size_t i=0;i<kUnknowns;++i) {
            std::size_t pivot=i;for(std::size_t row=i+1;row<kUnknowns;++row)if(std::abs(j[row][i])>std::abs(j[pivot][i]))pivot=row;
            if(!std::isfinite(j[pivot][i])||std::abs(j[pivot][i])<1e-18)return false;
            if(pivot!=i){std::swap(j[i],j[pivot]);std::swap(f[i],f[pivot]);}
            for(std::size_t row=i+1;row<kUnknowns;++row){const double scale=j[row][i]/j[i][i];for(std::size_t col=i+1;col<kUnknowns;++col)j[row][col]-=scale*j[i][col];f[row]-=scale*f[i];}
        }
        Unknowns delta{};
        for(int i=int(kUnknowns)-1;i>=0;--i){double v=f[std::size_t(i)];for(std::size_t k=std::size_t(i)+1;k<kUnknowns;++k)v-=j[std::size_t(i)][k]*delta[k];delta[std::size_t(i)]=v/j[std::size_t(i)][std::size_t(i)];}
        double scale=std::max(1.,std::abs(delta[0])/.1);
        for(std::size_t k=0;k<kUnknowns;++k){if(!std::isfinite(delta[k]))return false;scale=std::max(scale,std::abs(delta[k])/2);}
        bool accepted=false;double fraction=1/scale;
        for(std::size_t back=0;back<kSearch;++back,fraction*=.5) {
            auto candidate=y;Unknowns nf{};Matrix nj{};double ni=0;std::array<double,2> nd{};
            for(std::size_t k=0;k<kUnknowns;++k){candidate[k]-=fraction*delta[k];}
            ++evaluations;
            if(evaluate(candidate,base,dc,nf,nj,ni,nd)&&norm(nf)<oldNorm*(1-1e-4*fraction)){y=candidate;f=nf;j=nj;current=ni;diodes=nd;accepted=true;break;}
        }
        if(!accepted)return false;
    }
    return false;
}
bool Klon::prepare(float rate,KlonControls c,KlonRate mode) noexcept {
    if(!range(rate,mode==KlonRate::Reference1x?96000:44100,mode==KlonRate::Reference1x?1536000:96000)
       ||!range(c.gain,0,1)||!range(c.treble,0,1)||!range(c.level,0,1)
       ||(mode!=KlonRate::Reference1x&&mode!=KlonRate::Oversampled12x))return false;
    Klon next;next.controls_=c;next.mode_=mode;next.rate_=rate*(mode==KlonRate::Oversampled12x?kFactor:1);
    const auto cap=[&](std::size_t k,double f,double r,int p,int n){auto& x=next.caps_[k];x.farads=f;x.series=r;if(p>=0)x.a[std::size_t(p)]=1;if(n>=0)x.a[std::size_t(n)]=-1;};
    cap(0,100e-9,11000,-1,Input);cap(1,4.7e-6,0,Buffer,BypassHP);cap(2,100e-9,0,Buffer,Pre);
    cap(3,68e-9,0,Pre,CleanUpper);cap(4,68e-9,0,Pre,DrivePlus);cap(5,390e-9,1000,CleanUpper,Vref);
    cap(6,82e-9,0,Feedback,DriveMinus);cap(7,390e-12,0,DriveMinus,DriveOut);cap(8,1e-6,1000,DriveOut,Clip);
    cap(9,1e-6,0,Clip,PostClip);cap(10,2.2e-9,22000,PostClip,CleanWiper);cap(11,27e-9,12000,SumMinus,CleanWiper);
    cap(12,820e-12,0,SumMinus,SumOut);
    const double rt=4700+10000*c.treble,rb=1800+10000*(1-c.treble);
    cap(13,3.9e-9,rt*rb/(rt+rb),-1,ToneMinus);next.caps_[13].a[SumOut]=rt/(rt+rb);next.caps_[13].a[ToneOut]=rb/(rt+rb);
    cap(14,4.7e-6,560,ToneOut,LevelTop);cap(15,1e-6,0,FF1,-1);cap(16,47e-6,0,Vref,-1);
    if(!next.matrices(true))return false;
    Unknowns y{};double current=0,residual=0;uint32_t loops=0;std::array<double,2> diodes{};
    if(!next.solve(y,next.nodes_,true,residual,loops,current,diodes))return false;
    next.bias_=next.reconstruct(next.nodes_,y,current,true);next.biasUnknowns_=y;next.biasCurrent_=current;
    for(std::size_t k=0;k<kCaps;++k)next.capBias_[k]=dot(next.caps_[k].a,next.bias_);
    if(!next.matrices(false))return false;
    double sum=0;
    for(std::size_t i=0;i<next.fir_.size();++i){const double n=double(i)-(kTaps-1)/2.,cutoff=.4/kFactor;next.fir_[i]=float((n==0?2*cutoff:std::sin(2*pi*cutoff*n)/(pi*n))*(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1))));sum+=next.fir_[i]*(i+1==next.fir_.size()?1:2);}
    for(auto& v:next.fir_)v=float(v/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
void Klon::resetCircuit() noexcept {
    previous_=biasUnknowns_;for(std::size_t k=0;k<4;++k)oldU_[k]=biasUnknowns_[1+k];nodes_=bias_;diodes_={};residual_=kcl_=0;
    capAmps_={};mix_={};paths_={};oldMix_={};for(std::size_t c=0;c<kCaps;++c){caps_[c].z=0;capVolts_[c]=capBias_[c];}
}
void Klon::reset() noexcept {
    resetCircuit();up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;
    failures_=invalid_=inputClamps_=outputClamps_=evaluations_=solves_=maximum_=rails_=slews_=0;running_=prepared_;
}
double Klon::sample(double input) noexcept {
    Nodes base=bias_;
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t c=0;c<kCaps;++c)base[i]+=history_[i][c]*(caps_[c].z-(c==0?input:0));
    auto y=previous_;double current=0,residual=0;uint32_t loops=0;std::array<double,2> diodes{};
    bool ok=solve(y,base,false,residual,loops,current,diodes);count(solves_);count(evaluations_,loops);maximum_=std::max(maximum_,loops);
    const auto n=reconstruct(base,y,current,false);Nodes change{};
    std::array<double,kCaps> voltage{},history{},amps{};
    if(ok) {
        for(std::size_t i=0;i<kNodes;++i){ok=ok&&range(n[i],-24,24);change[i]=n[i]-bias_[i];}
        for(std::size_t c=0;c<kCaps;++c){const auto& x=caps_[c];const double drive=dot(x.a,change)+(c==0?input:0);amps[c]=x.g*(drive-x.z);const double delta=drive-x.series*amps[c];voltage[c]=capBias_[c]+delta;history[c]=2*delta-x.z;ok=ok&&range(voltage[c],-48,48)&&range(history[c],-96,96);}
    }
    if(!ok){count(failures_);resetCircuit();residual_=residual;return 0;}
    for(std::size_t k=0;k<4;++k){const double lo=k<2?-3:-11.6,hi=k<2?3:10.2;const double raw=klon_detail::wp*(klon_detail::gain*(n[positive[k]]-n[negative[k]])-y[1+k]);if(y[1+k]<lo||y[1+k]>hi)count(rails_);if(std::abs(raw)>=klon_detail::slew)count(slews_);oldU_[k]=previous_[1+k];}
    previous_=y;nodes_=n;capVolts_=voltage;capAmps_=amps;diodes_=diodes;residual_=residual;
    for(std::size_t c=0;c<kCaps;++c)caps_[c].z=history[c];
    mix_={amps[9],(n[FF1]-n[SumMinus])/15000,-amps[10]+(n[CleanWiper]-n[SumMinus])/27000-amps[11]};
    kcl_=mix_[0]+mix_[1]+mix_[2]+(n[SumOut]-n[SumMinus])/392e3-amps[12]-(n[SumMinus]-n[Vref])/1e12;
    const std::array<double,3> dc={0,(bias_[FF1]-bias_[SumMinus])/15000,(bias_[CleanWiper]-bias_[SumMinus])/27000};
    const double q=2*rate_*392e3*820e-12,a=(q-1)/(q+1),b=-392e3/(q+1);
    for(std::size_t k=0;k<3;++k){const double in=mix_[k]-dc[k];paths_[k]=a*paths_[k]+b*(in+oldMix_[k]);oldMix_[k]=in;}
    return controls_.level==0?0:n[Jack];
}
float Klon::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}
    if(std::abs(input)>2){input=std::clamp(input,-2.f,2.f);count(inputClamps_);}
    double result=0;
    if(mode_==KlonRate::Reference1x)result=sample(input);
    else {
        upIndex_=upIndex_==0?uint32_t(up_.size()-1):upIndex_-1;up_[upIndex_]=input;
        for(std::size_t phase=0;phase<kFactor;++phase){float interpolated=0;auto index=upIndex_;for(std::size_t tap=phase;tap<kTaps;tap+=kFactor){interpolated+=coefficient(tap)*up_[index];if(++index==up_.size())index=0;}downIndex_=downIndex_==0?kTaps-1:downIndex_-1;down_[downIndex_]=float(sample(kFactor*interpolated));if(phase==0){index=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){result+=coefficient(tap)*down_[index];if(++index==kTaps)index=0;}}}
    }
    result/=8; // fixed software headroom, not electrical Seed3 calibration
    if(!std::isfinite(result)){count(failures_);resetCircuit();return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}
    return float(result);
}
} // namespace parable::dsp
