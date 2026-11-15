#pragma once
// Nominal MIT ACME Ebers-Moll transport foundation via main Muff/TS808.
// Copyright2015/2016/2017 Martin Holters; third_party/capricorn/ACME-LICENSE.md.
// Circuit-specific stopped preparation only, never called from process().
#include "parable_capricorn.h"
#include <algorithm>
#include <cmath>
namespace parable::dsp::capricorn_detail {
template<unsigned N> using V=std::array<double,N>;
template<unsigned N> using M=std::array<V<N>,N>;
template<unsigned N> bool linear(M<N> a,V<N> b,V<N>& x) noexcept {
    for(unsigned i=0;i<N;++i){unsigned pivot=i;for(unsigned j=i+1;j<N;++j)if(std::abs(a[j][i])>std::abs(a[pivot][i]))pivot=j;
        if(!std::isfinite(a[pivot][i])||std::abs(a[pivot][i])<1e-17)return false;
        std::swap(a[pivot],a[i]);std::swap(b[pivot],b[i]);
        for(unsigned j=i+1;j<N;++j){const auto f=a[j][i]/a[i][i];for(unsigned k=i+1;k<N;++k)a[j][k]-=f*a[i][k];b[j]-=f*b[i];}}
    for(int i=int(N)-1;i>=0;--i){double y=b[unsigned(i)];for(unsigned j=unsigned(i)+1;j<N;++j)y-=a[unsigned(i)][j]*x[j];x[unsigned(i)]=y/a[unsigned(i)][unsigned(i)];}return true;
}
constexpr double vt=1.380649e-23*300/1.602176634e-19,diodeIs=2.52e-9,nv=1.752*vt;
inline V<3> bjt(const V<3>& v) noexcept {const double f=1e-14*std::expm1(std::clamp((v[0]-v[2])/vt,-60.,40.)),r=1e-14*std::expm1(std::clamp((v[0]-v[1])/vt,-60.,40.));const double ib=f/350+r/4,ic=f-1.25*r;return {ib,ic,-ib-ic};}
struct Q2 {
    V<3> bias{.65,3.7,.012};
    V<3> residual(const V<3>& v,double input,bool dc) const noexcept {
        const auto i=bjt(v);if(dc)return {v[0]/100000+(v[0]-v[1])/470000+i[0],(v[1]-9)/10000+(v[1]-v[0])/470000+i[1],v[2]/22+i[2]};
        const auto ib=bjt(bias);V<3> q{};for(unsigned k=0;k<3;++k)q[k]=v[k]-bias[k];
        return {q[0]/100000+(q[0]-q[1])/470000+(q[0]-input)/550+i[0]-ib[0],q[1]/10000+(q[1]-q[0])/470000+q[1]/100000+i[1]-ib[1],q[2]/22+i[2]-ib[2]};
    }
    M<3> jacobian(const V<3>& v,bool dc=false) const noexcept {M<3> j{};for(unsigned k=0;k<3;++k){auto a=v,b=v;a[k]+=1e-6;b[k]-=1e-6;const auto p=residual(a,0,dc),m=residual(b,0,dc);for(unsigned n=0;n<3;++n)j[n][k]=(p[n]-m[n])/2e-6;}return j;}
    bool solve(V<3>& v,double input=0,bool dc=false) const noexcept {
        for(unsigned n=0;n<100;++n){const auto f=residual(v,input,dc);double norm=0;for(auto x:f)norm=std::max(norm,std::abs(x));if(norm<1e-12)return true;
            V<3> rhs{},step{};for(unsigned k=0;k<3;++k)rhs[k]=-f[k];if(!linear<3>(jacobian(v,dc),rhs,step))return false;
            double size=0;for(auto x:step)size=std::max(size,std::abs(x));double scale=std::min(1.,.25/std::max(size,1e-30));bool accepted=false;
            for(unsigned trial=0;trial<16;++trial){auto w=v;for(unsigned k=0;k<3;++k)w[k]+=scale*step[k];double next=0;for(auto x:residual(w,input,dc))next=std::max(next,std::abs(x));if(std::isfinite(next)&&next<norm){v=w;accepted=true;break;}scale*=.5;}if(!accepted)return false;
        }return false;
    }
};
// Eliminate just this circuit's passive companion network off-thread.
template<unsigned Nodes,unsigned Caps> bool compile(M<Nodes> g,V<Nodes> drive,const std::array<V<Nodes>,Caps>& t,const std::array<double,Caps>& caps,double fs,unsigned output,unsigned diode,Prepared<Caps>& p) noexcept {
    std::array<double,Caps> cg{};for(unsigned k=0;k<Caps;++k){cg[k]=2*fs*caps[k];for(unsigned a=0;a<Nodes;++a)for(unsigned b=0;b<Nodes;++b)g[a][b]+=cg[k]*t[k][a]*t[k][b];}
    V<Nodes> u{},j{},rhs{};rhs[diode]=1;if(!linear<Nodes>(g,drive,u)||!linear<Nodes>(g,rhs,j)||j[diode]<=0)return false;
    p.D=float(u[output]);p.Ud=float(u[diode]);p.resistance=float(j[diode]);p.Ky=float(j[output]/j[diode]);
    for(unsigned k=0;k<Caps;++k){double b=0,z=0;for(unsigned a=0;a<Nodes;++a){b+=t[k][a]*u[a];z+=t[k][a]*j[a];}if(caps[k]>0){p.B[k]=float(2*b);p.K[k]=float(2*z/j[diode]);}}
    for(unsigned k=0;k<Caps;++k){rhs={};for(unsigned a=0;a<Nodes;++a)rhs[a]=cg[k]*t[k][a];V<Nodes> v{};if(!linear<Nodes>(g,rhs,v))return false;p.C[k]=float(v[output]);p.H[k]=float(v[diode]);
        for(unsigned n=0;n<Caps;++n){double z=0;for(unsigned a=0;a<Nodes;++a)z+=t[n][a]*v[a];if(caps[n]>0)p.A[n][k]=float(2*z-(n==k?1:0));}}
    return true;
}
inline bool tone(double fs,CapricornControls c,Prepared<4>& p) noexcept {
    // X,C9clip,Z,bright,dark,ToneWiper,LevelWiper,ground. Merge exact pot endpoints.
    unsigned parent[8];for(unsigned k=0;k<8;++k)parent[k]=k;
    auto root=[&](unsigned a){while(parent[a]!=a)a=parent[a];return a;};
    auto join=[&](unsigned a,unsigned b){a=root(a);b=root(b);parent[a]=b;};
    if(c.tone==0)join(5,4);if(c.tone==1)join(5,3);if(c.level==0)join(6,7);if(c.level==1)join(5,6);
    int map[8],assigned[8];std::fill_n(assigned,8,-2);unsigned count=0;const auto ground=root(7);
    for(unsigned k=0;k<8;++k){const auto r=root(k);if(r==ground)map[k]=-1;else {if(assigned[r]==-2)assigned[r]=int(count++);map[k]=assigned[r];}}
    M<7> g{};for(unsigned k=count;k<7;++k)g[k][k]=1;V<7> drive{};
    auto stamp=[&](unsigned a,unsigned b,double resistance){if(resistance==0)return;const int i=map[a],j=map[b];const double z=1/resistance;if(i>=0)g[unsigned(i)][unsigned(i)]+=z;if(j>=0)g[unsigned(j)][unsigned(j)]+=z;if(i>=0&&j>=0){g[unsigned(i)][unsigned(j)]-=z;g[unsigned(j)][unsigned(i)]-=z;}};
    stamp(0,7,2200);drive[unsigned(map[0])]=1./2200;stamp(2,3,2200);stamp(3,7,6800);stamp(1,4,6800);
    stamp(3,5,20000*(1-c.tone));stamp(4,5,20000*c.tone);stamp(5,6,100000*(1-c.level));stamp(6,7,100000*c.level);stamp(6,7,1e6);
    std::array<V<7>,4> t{};const unsigned a[]={0,1,1,4},b[]={1,7,2,7};for(unsigned k=0;k<4;++k){if(map[a[k]]>=0)t[k][unsigned(map[a[k]])]+=1;if(map[b[k]]>=0)t[k][unsigned(map[b[k]])]-=1;}
    // Level0 is an exact grounded output; use clip as temporary output, then zero.
    const auto output=map[6]<0?unsigned(map[1]):unsigned(map[6]);const bool ok=compile<7,4>(g,drive,t,{.47e-6,10e-9,22e-9,100e-9},fs,output,unsigned(map[1]),p);
    if(map[6]<0){p.D=p.Ky=0;p.C.fill(0);}return ok;
}
}
