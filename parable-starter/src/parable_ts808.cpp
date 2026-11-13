#include "parable_ts808.h"
#include "ts808_device.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr std::array<std::size_t,5> pos={Ts808::B1,Ts808::B1,Ts808::B3,Ts808::B3,Ts808::Clip};
constexpr std::array<int,5> neg={Ts808::E1,-1,Ts808::E3,-1,Ts808::Minus};
constexpr std::array<std::size_t,2> plus={Ts808::Plus,Ts808::TonePlus},minus={Ts808::Minus,Ts808::ToneMinus};
bool range(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
void count(uint32_t& n,uint32_t add=1) noexcept {n+=std::min(add,std::numeric_limits<uint32_t>::max()-n);}
double dot(const Ts808::Nodes& a,const Ts808::Nodes& b) noexcept {double v=0;for(std::size_t i=0;i<a.size();++i)v+=a[i]*b[i];return v;}
}
bool Ts808::matrices(bool dc) noexcept {
    // One fixed selected schematic. Exact stopped-only elimination, not a netlist loader.
    std::array<Nodes,kNodes> a{},inverse{};
    std::array<Currents,kNodes> inject{}; Nodes rhs{};
    const auto resistor=[&](std::size_t p,int n,double r,double rail=0) {
        const double g=1/r; a[p][p]+=g;
        if(n>=0){a[std::size_t(n)][std::size_t(n)]+=g;a[p][std::size_t(n)]-=g;a[std::size_t(n)][p]-=g;}
        else rhs[p]+=g*rail;
    };
    resistor(B1,-1,510e3,4.5); resistor(E1,-1,10e3);
    resistor(Plus,-1,10e3,4.5); resistor(Plus,Minus,5e6);
    const double pot=(std::pow(20.,controls_.drive)-1)/19.;
    resistor(Minus,Clip,51e3+500e3*pot); resistor(Clip,-1,75,4.5);
    resistor(Clip,TonePlus,1e3); resistor(TonePlus,-1,10e3,4.5);
    resistor(TonePlus,ToneMinus,20e3); resistor(ToneMinus,ToneOut,1e3);
    resistor(TonePlus,ToneMinus,5e6); resistor(ToneOut,-1,75,4.5);
    resistor(LevelTop,-1,100e3); resistor(B3,-1,510e3,4.5);
    resistor(E3,-1,10e3); resistor(Jack,-1,10e3); resistor(Jack,-1,1e6);
    for(std::size_t q=0;q<2;++q) {
        const auto b=q?B3:B1,e=q?E3:E1;
        inject[b][2*q]=1/ts808_detail::bf; inject[e][2*q]=-(1+1/ts808_detail::bf);
        inject[b][2*q+1]=1/ts808_detail::br; inject[e][2*q+1]=1;
    }
    inject[Clip][4]=1; inject[Minus][4]=-1;
    for(auto& c:caps_) {
        c.g=dc?0:1/(c.series+1/(2*rate_*c.farads));
        for(std::size_t i=0;i<kNodes;++i)for(std::size_t j=0;j<kNodes;++j)a[i][j]+=c.g*c.a[i]*c.a[j];
    }
    for(std::size_t i=0;i<kNodes;++i)inverse[i][i]=1;
    for(std::size_t i=0;i<kNodes;++i) {
        std::size_t pivot=i;
        for(std::size_t j=i+1;j<kNodes;++j)if(std::abs(a[j][i])>std::abs(a[pivot][i]))pivot=j;
        if(!std::isfinite(a[pivot][i])||std::abs(a[pivot][i])<1e-18)return false;
        if(pivot!=i){std::swap(a[i],a[pivot]);std::swap(inverse[i],inverse[pivot]);}
        const double div=a[i][i];
        for(std::size_t j=0;j<kNodes;++j){a[i][j]/=div;inverse[i][j]/=div;}
        for(std::size_t row=0;row<kNodes;++row)if(row!=i) {
            const double scale=a[row][i];
            for(std::size_t col=0;col<kNodes;++col){a[row][col]-=scale*a[i][col];inverse[row][col]-=scale*inverse[i][col];}
        }
    }
    nodes_={}; current_={}; history_={}; drive_={};
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t n=0;n<kNodes;++n) {
        nodes_[i]+=inverse[i][n]*rhs[n];
        for(std::size_t p=0;p<kJunctions;++p)current_[i][p]+=inverse[i][n]*inject[n][p];
        for(std::size_t c=0;c<kCaps;++c)history_[i][c]+=inverse[i][n]*caps_[c].a[n]*caps_[c].g;
    }
    for(std::size_t i=0;i<kNodes;++i){drive_[i][0]=inverse[i][Clip]/75;drive_[i][1]=inverse[i][ToneOut]/75;}
    return true;
}
Ts808::Nodes Ts808::reconstruct(const Nodes& base,const Unknowns& y,const Currents& current,bool dc) const noexcept {
    Nodes n=base;
    for(std::size_t i=0;i<kNodes;++i) {
        for(std::size_t p=0;p<kJunctions;++p)n[i]-=current_[i][p]*(current[p]-(dc?0:biasCurrents_[p]));
        for(std::size_t k=0;k<2;++k)n[i]+=drive_[i][k]*(y[5+k]-(dc?0:biasUnknowns_[5+k]));
    }
    return n;
}
bool Ts808::evaluate(Unknowns& y,const Nodes& base,bool dc,Unknowns& f,Matrix& j,Currents& current,std::array<double,2>& diodes) const noexcept {
    const auto device=ts808_detail::currents(y,controls_.openD2);if(!device.valid)return false;
    // Exactly eliminate both piecewise-linear amplifier states for the given
    // nonlinear junction currents. Enumerate at most5x5 affine regions; no
    // iterative interrogation of a saturated amplifier or changed device law.
    Unknowns zero=y;zero[5]=zero[6]=0;
    const auto n0=reconstruct(base,zero,device.i,dc);
    std::array<std::array<double,2>,2> errorDrive{};
    std::array<double,2> errorBase{};
    std::array<std::array<std::array<double,2>,5>,2> regions{};
    // BDF2 internal pole states: second-order and stiffly stable, with the
    // same continuous op-amp law. External passive caps remain trapezoidal.
    const double step=2/(3*rate_);
    std::array<double,2> ampPrevious{};
    for(std::size_t a=0;a<2;++a)ampPrevious[a]=(4*previous_[5+a]-oldU_[a])/3;
    for(std::size_t a=0;a<2;++a) {
        errorBase[a]=n0[plus[a]]-n0[minus[a]];
        for(std::size_t k=0;k<2;++k)errorDrive[a][k]=drive_[plus[a]][k]-drive_[minus[a]][k];
        const double den=dc?ts808_detail::wp:1+step*ts808_detail::wp;
        const double coefficient=dc?ts808_detail::wp*ts808_detail::gain:step*ts808_detail::wp*ts808_detail::gain;
        const double prev=dc?0:ampPrevious[a],g=dc?ts808_detail::railG:step*ts808_detail::railG;
        regions[a][0]={coefficient/den,prev/den};
        for(std::size_t sign=0;sign<2;++sign) {
            const double edge=sign==0?3:-3;
            regions[a][1+sign]={coefficient/(den+g),(prev+edge*g)/(den+g)};
            const double target=prev+(sign==0?1:-1)*step*ts808_detail::slew;
            const double rail=std::clamp(target,-3.,3.);
            regions[a][3+sign]={0,dc?(sign==0?1:-1)*(3+ts808_detail::slew/ts808_detail::railG):rail+(target-rail)/(1+(target==rail?0:g))};
        }
    }
    std::array<std::array<double,2>,2> inverse{};std::array<double,2> slopes{};bool found=false;
    for(std::size_t a=0;a<5&&!found;++a)for(std::size_t b=0;b<5&&!found;++b) {
        const double h0=regions[0][a][0],h1=regions[1][b][0];
        const double m00=1-h0*errorDrive[0][0],m01=-h0*errorDrive[0][1];
        const double m10=-h1*errorDrive[1][0],m11=1-h1*errorDrive[1][1],det=m00*m11-m01*m10;
        if(!std::isfinite(det)||std::abs(det)<1e-18)continue;
        const double r0=regions[0][a][1]+h0*errorBase[0],r1=regions[1][b][1]+h1*errorBase[1];
        const std::array<double,2> u={(r0*m11-r1*m01)/det,(m00*r1-m10*r0)/det};
        const auto p0=ts808_detail::response(errorBase[0]+errorDrive[0][0]*u[0]+errorDrive[0][1]*u[1],ampPrevious[0],step,dc);
        const auto p1=ts808_detail::response(errorBase[1]+errorDrive[1][0]*u[0]+errorDrive[1][1]*u[1],ampPrevious[1],step,dc);
        if(std::abs(u[0]-p0.value)>1e-8||std::abs(u[1]-p1.value)>1e-8)continue;
        y[5]=u[0];y[6]=u[1];slopes={h0,h1};inverse={{{m11/det,-m01/det},{-m10/det,m00/det}}};found=true;
    }
    if(!found)return false;
    const auto n=reconstruct(base,y,device.i,dc);j={};f={};
    std::array<std::array<double,kJunctions>,2> du{};
    for(std::size_t k=0;k<kJunctions;++k) {
        const double e0=-slopes[0]*(current_[plus[0]][k]-current_[minus[0]][k]);
        const double e1=-slopes[1]*(current_[plus[1]][k]-current_[minus[1]][k]);
        du[0][k]=inverse[0][0]*e0+inverse[0][1]*e1;du[1][k]=inverse[1][0]*e0+inverse[1][1]*e1;
    }
    for(std::size_t p=0;p<kJunctions;++p) {
        f[p]=y[p]-n[pos[p]]+(neg[p]<0?9:n[std::size_t(neg[p])]);
        for(std::size_t k=0;k<kJunctions;++k) {
            double derivative=-current_[pos[p]][k];
            for(std::size_t a=0;a<2;++a)derivative+=drive_[pos[p]][a]*du[a][k];
            if(neg[p]>=0) {
                derivative+=current_[std::size_t(neg[p])][k];
                for(std::size_t a=0;a<2;++a)derivative-=drive_[std::size_t(neg[p])][a]*du[a][k];
            }
            j[p][k]=-derivative*device.g[k];
        }
        j[p][p]+=1;
    }
    for(double v:f)if(!std::isfinite(v))return false;
    current=device.i;diodes=device.diodes;return true;
}
bool Ts808::solve(Unknowns& y,const Nodes& base,bool dc,double& residual,uint32_t& evaluations,Currents& current,std::array<double,2>& diodes) const noexcept {
    const uint32_t budget=dc?120:kUpdates;const double tolerance=dc?1e-12:1e-9;
    Matrix j{};Unknowns f{};evaluations=1;
    if(!evaluate(y,base,dc,f,j,current,diodes))return false;
    const auto norm=[](const Unknowns& v){double sum=0;for(double x:v)sum+=x*x;return sum;};
    for(uint32_t update=0;update<=budget;++update) {
        residual=0;for(double v:f)residual=std::max(residual,std::abs(v));
        if(residual<tolerance)return true;
        if(update==budget)return false;
        const double previousNorm=norm(f);
        // Main's owned partial-pivot Newton/forward-junction damping pattern.
        for(std::size_t i=0;i<kJunctions;++i) {
            std::size_t pivot=i;
            for(std::size_t row=i+1;row<kJunctions;++row)if(std::abs(j[row][i])>std::abs(j[pivot][i]))pivot=row;
            if(!std::isfinite(j[pivot][i])||std::abs(j[pivot][i])<1e-18)return false;
            if(pivot!=i){std::swap(j[i],j[pivot]);std::swap(f[i],f[pivot]);}
            for(std::size_t row=i+1;row<kJunctions;++row) {
                const double scale=j[row][i]/j[i][i];
                for(std::size_t col=i+1;col<kJunctions;++col)j[row][col]-=scale*j[i][col];
                f[row]-=scale*f[i];
            }
        }
        Unknowns delta{};
        for(int i=int(kJunctions)-1;i>=0;--i) {
            double v=f[std::size_t(i)];
            for(std::size_t k=std::size_t(i)+1;k<kJunctions;++k)v-=j[std::size_t(i)][k]*delta[k];
            delta[std::size_t(i)]=v/j[std::size_t(i)][std::size_t(i)];
        }
        double scale=1;
        for(std::size_t i=0;i<kJunctions;++i) {
            if(!std::isfinite(delta[i]))return false;
            if(i<4||(i==4&&!controls_.openD2))scale=std::max(scale,-delta[i]/std::max(.05,.05-y[i]));
            if(i==4)scale=std::max(scale,delta[i]/std::max(.05,.05+y[i]));
            scale=std::max(scale,std::abs(delta[i]));
        }
        bool accepted=false;
        double fraction=1/scale;
        for(std::size_t backtrack=0;backtrack<kSearch;++backtrack,fraction*=.5) {
            Unknowns candidate=y,newF{};Matrix newJ{};Currents newCurrent{};std::array<double,2> newDiodes{};
            for(std::size_t i=0;i<kJunctions;++i)candidate[i]-=fraction*delta[i];
            ++evaluations;
            if(evaluate(candidate,base,dc,newF,newJ,newCurrent,newDiodes)
               &&norm(newF)<previousNorm*(1-1e-4*fraction)) {
                y=candidate;f=newF;j=newJ;current=newCurrent;diodes=newDiodes;accepted=true;break;
            }
        }
        if(!accepted)return false;
    }
    return false;
}
bool Ts808::prepare(float rate,Ts808Controls c,Ts808Rate mode) noexcept {
    if(!range(rate,mode==Ts808Rate::Reference1x?96000:44100,mode==Ts808Rate::Reference1x?1536000:96000)
       ||!range(c.drive,0,1)||!range(c.tone,0,1)||!range(c.level,0,1)
       ||(mode!=Ts808Rate::Reference1x&&mode!=Ts808Rate::Oversampled16x))return false;
    Ts808 next;next.controls_=c;next.mode_=mode;next.rate_=rate*(mode==Ts808Rate::Oversampled16x?kFactor:1);
    const auto cap=[&](std::size_t index,double farads,double series,int p,int n) {
        auto& x=next.caps_[index];x.farads=farads;x.series=series;
        if(p>=0)x.a[std::size_t(p)]=1;
        if(n>=0)x.a[std::size_t(n)]=-1;
    };
    cap(0,20e-9,2e3,-1,B1); cap(1,1e-6,0,E1,Plus); cap(2,47e-9,4.7e3,Minus,-1);
    cap(3,51e-12,0,Minus,Clip); cap(4,220e-9,0,TonePlus,-1);
    cap(5,220e-9,220+20e3*c.tone*(1-c.tone),TonePlus,-1);
    next.caps_[5].a[TonePlus]=1-c.tone;next.caps_[5].a[ToneMinus]=c.tone;
    cap(6,1e-6,1e3,ToneOut,LevelTop);
    cap(7,100e-9,100e3*c.level*(1-c.level),LevelTop,B3);next.caps_[7].a[LevelTop]=c.level;
    cap(8,10e-6,100,E3,Jack);
    if(!next.matrices(true))return false;
    Unknowns y{.63,-5,.63,-5,0,0,0};Currents current{};std::array<double,2> diodes{};double residual=0;uint32_t loops=0;
    if(!next.solve(y,next.nodes_,true,residual,loops,current,diodes))return false;
    next.bias_=next.reconstruct(next.nodes_,y,current,true);next.biasUnknowns_=y;next.biasCurrents_=current;
    for(std::size_t k=0;k<kCaps;++k)next.capBias_[k]=dot(next.caps_[k].a,next.bias_);
    if(!next.matrices(false))return false;
    double sum=0;
    for(std::size_t i=0;i<next.fir_.size();++i) {
        const double n=double(i)-(kTaps-1)/2.,cutoff=.4/kFactor;
        next.fir_[i]=float((n==0?2*cutoff:std::sin(2*pi*cutoff*n)/(pi*n))*(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1))));
        sum+=next.fir_[i]*(i+1==next.fir_.size()?1:2);
    }
    for(auto& v:next.fir_)v=float(v/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
void Ts808::resetCircuit() noexcept {
    previous_=biasUnknowns_;oldU_={biasUnknowns_[5],biasUnknowns_[6]};nodes_=bias_;diodes_={};residual_=0;
    for(std::size_t c=0;c<kCaps;++c){caps_[c].z=0;capVolts_[c]=capBias_[c];}
}
void Ts808::reset() noexcept {
    resetCircuit();up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;
    failures_=outputClamps_=inputClamps_=invalid_=evaluations_=solves_=maximum_=railSamples_=slewSamples_=0;running_=prepared_;
}
double Ts808::sample(double input) noexcept {
    Nodes base=bias_;
    for(std::size_t i=0;i<kNodes;++i)for(std::size_t c=0;c<kCaps;++c)base[i]+=history_[i][c]*(caps_[c].z-(c==0?input:0));
    Unknowns y=previous_;Currents current{};std::array<double,2> diodes{};double residual=0;uint32_t loops=0;
    bool ok=solve(y,base,false,residual,loops,current,diodes);
    count(solves_);count(evaluations_,loops);maximum_=std::max(maximum_,loops);
    const auto n=reconstruct(base,y,current,false);
    std::array<double,kCaps> voltage{},history{};
    if(ok) {
        for(double v:n)ok=ok&&range(v,-16,16);
        for(std::size_t c=0;c<kCaps;++c) {
            const auto& part=caps_[c];Nodes change{};
            for(std::size_t i=0;i<kNodes;++i)change[i]=n[i]-bias_[i];
            const double drive=dot(part.a,change)+(c==0?input:0),amps=part.g*(drive-part.z);
            const double delta=drive-part.series*amps;voltage[c]=capBias_[c]+delta;history[c]=2*delta-part.z;
            ok=ok&&range(voltage[c],-32,32)&&range(history[c],-64,64);
        }
    }
    if(!ok){count(failures_);resetCircuit();residual_=residual;return 0;}
    for(std::size_t a=0;a<2;++a) {
        const auto amp=ts808_detail::amplifier(n[plus[a]]-n[minus[a]],y[5+a]);
        if(amp.rail)count(railSamples_);
        if(amp.limited)count(slewSamples_);
    }
    oldU_={previous_[5],previous_[6]};previous_=y;nodes_=n;capVolts_=voltage;residual_=residual;
    diodes_=diodes;
    for(std::size_t c=0;c<kCaps;++c)caps_[c].z=history[c];
    // Level0 disconnects the signal path in this topology: return its exact
    // zero rather than DC-solver roundoff. All internal states still advance.
    return controls_.level==0?0:n[Jack];
}
float Ts808::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}
    if(std::abs(input)>2){input=std::clamp(input,-2.f,2.f);count(inputClamps_);}
    double result=0;
    if(mode_==Ts808Rate::Reference1x)result=sample(input);
    else {
        upIndex_=upIndex_==0?uint32_t(up_.size()-1):upIndex_-1;up_[upIndex_]=input;
        for(std::size_t phase=0;phase<kFactor;++phase) {
            float interpolated=0;auto index=upIndex_;
            for(std::size_t tap=phase;tap<kTaps;tap+=kFactor){interpolated+=coefficient(tap)*up_[index];if(++index==up_.size())index=0;}
            downIndex_=downIndex_==0?kTaps-1:downIndex_-1;down_[downIndex_]=float(sample(kFactor*interpolated));
            if(phase==0){index=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){result+=coefficient(tap)*down_[index];if(++index==kTaps)index=0;}}
        }
    }
    result/=4; // fixed declared software headroom, not ADC/DAC voltage calibration
    if(!std::isfinite(result)){count(failures_);resetCircuit();return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}
    return float(result);
}
} // namespace parable::dsp
