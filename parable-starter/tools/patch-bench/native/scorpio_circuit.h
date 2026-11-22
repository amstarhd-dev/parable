#pragma once
// Stopped-only five-node EM specialization. MIT ACME transport-current foundation
// via main's Muff/wavefolder; copyright2015/2016/2017 Martin Holters, notice in
// third_party/scorpio/ACME-LICENSE.md. No audio-thread transistor solve.
#include <array>
#include <algorithm>
#include <cmath>
namespace parable::dsp::scorpio_detail {
template<unsigned N> using Vector=std::array<double,N>;
template<unsigned N> using Matrix=std::array<Vector<N>,N>;
using Vec=Vector<5>;using Mat=Matrix<5>;
template<unsigned N> inline bool linear(Matrix<N> a,Vector<N> rhs,Vector<N>& out) noexcept {
    for(unsigned i=0;i<N;++i){unsigned p=i;for(unsigned j=i+1;j<N;++j)if(std::abs(a[j][i])>std::abs(a[p][i]))p=j;
        if(!std::isfinite(a[p][i])||std::abs(a[p][i])<1e-17)return false;
        if(p!=i){std::swap(a[p],a[i]);std::swap(rhs[p],rhs[i]);}
        for(unsigned j=i+1;j<N;++j){const double s=a[j][i]/a[i][i];for(unsigned k=i+1;k<N;++k)a[j][k]-=s*a[i][k];rhs[j]-=s*rhs[i];}}
    for(int i=int(N)-1;i>=0;--i){double v=rhs[unsigned(i)];for(unsigned j=unsigned(i)+1;j<N;++j)v-=a[unsigned(i)][j]*out[j];out[unsigned(i)]=v/a[unsigned(i)][unsigned(i)];}
    return true;
}
struct Terminals {double b=0,c=0,e=0;};
inline Terminals transistor(double b,double c,double e,bool pnp) noexcept {
    constexpr double vt=1.380649e-23*300/1.602176634e-19;
    const double sign=pnp?-1:1,Is=pnp?1e-9:1e-14,beta=pnp?100:400;
    const double f=Is*std::expm1(std::clamp(sign*(b-e)/vt,-80.,40.));
    const double r=Is*std::expm1(std::clamp(sign*(b-c)/vt,-80.,40.));
    Terminals t;t.b=sign*(f/beta+r/4);t.c=sign*(f-1.25*r)-(pnp?50e-6:0);t.e=-t.b-t.c;return t;
}
struct Circuit {
    double r3=5600,feedback=5600,lowResistance=3500;Vec bias{2.91,6.28,2.28,6.62,2.81};
    std::array<double,2> rails(const Vec& v) const noexcept {
        const double a=.001+1/r3+1./56000,b=-1./56000,d=1./56000+1./33000+1./220000;
        const double u=.009+v[1]/r3,w=v[0]/220000,det=a*d-b*b;
        return {(u*d-b*w)/det,(a*w-b*u)/det};
    }
    Vec residual(const Vec& v,double input,bool dc) const noexcept {
        const auto n=transistor(v[0],v[1],v[2],false),p=transistor(v[1],v[4],v[3],true);
        if(dc){const auto rail=rails(v);return {(v[0]-rail[1])/220000+n.b,(v[1]-rail[0])/r3+n.c+p.b,v[2]/5668+n.e,(v[3]-9)/4700+p.e,v[4]/5600+p.c};}
        const auto nb=transistor(bias[0],bias[1],bias[2],false),pb=transistor(bias[1],bias[4],bias[3],true);
        Vec q{};for(unsigned k=0;k<5;++k)q[k]=v[k]-bias[k];const double fb=(q[2]-q[4])/feedback;
        return {(q[0]-input)/1000+q[0]/220000+n.b-nb.b,q[1]/r3+n.c-nb.c+p.b-pb.b,
                q[2]/68+fb+n.e-nb.e,q[3]*(1./4700+1/lowResistance)+p.e-pb.e,q[4]/5600-fb+p.c-pb.c};
    }
    Mat jacobian(const Vec& v,bool dc=false) const noexcept {
        Mat a{};for(unsigned j=0;j<5;++j){auto hi=v,lo=v;hi[j]+=1e-6;lo[j]-=1e-6;const auto f=residual(hi,0,dc),g=residual(lo,0,dc);for(unsigned i=0;i<5;++i)a[i][j]=(f[i]-g[i])/2e-6;}return a;
    }
    bool solve(Vec& v,double input,bool dc=false) const noexcept {
        for(unsigned iteration=0;iteration<100;++iteration){const auto f=residual(v,input,dc);double norm=0;for(double z:f)norm=std::max(norm,std::abs(z));if(norm<1e-12)return true;
            Vec rhs{},step{};for(unsigned i=0;i<5;++i)rhs[i]=-f[i];if(!linear<5>(jacobian(v,dc),rhs,step))return false;
            double maxStep=0;for(double z:step)maxStep=std::max(maxStep,std::abs(z));double scale=std::min(1.,.15/std::max(maxStep,1e-30));bool ok=false;
            for(unsigned search=0;search<16;++search){auto trial=v;for(unsigned i=0;i<5;++i)trial[i]+=scale*step[i];double n=0;for(double z:residual(trial,input,dc))n=std::max(n,std::abs(z));if(std::isfinite(n)&&n<norm){v=trial;ok=true;break;}scale*=.5;}
            if(!ok)return false;
        }return false;
    }
};
}
