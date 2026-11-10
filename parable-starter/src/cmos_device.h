#pragma once
// Narrow adaptation of ACME.jl mosfet nonlinear current/Jacobian, MIT Expat:
// HSU-ANT/ACME.jl 3832ccafb952ee2ca0a27286c091d613062c23fd src/elements.jl.
// Copyright (c) 2015, 2016, 2017 Martin Holters; exact notice in
// third_party/cmos/ACME-LICENSE.md. Fit facts: Koeper/Holters DAFx2020, CC-BY-3.0.
#include <algorithm>
namespace parable::dsp::cmos_detail {
struct Current { double value, gateDerivative, drainDerivative; };
inline Current mos(double u, double d, bool p) noexcept {
    // Fit functions were measured for |Vgs|=2.5..6 V. Freeze their parameters
    // outside that interval; actual gate voltage remains in the MOS equation.
    const double q=std::clamp(u,2.5,6.);
    const bool fitted=u>=2.5&&u<=6;
    double alpha, da, threshold, dt, lambda;
    if(p){
        alpha=((1.0800821774906936e-5*q-.00016848836814836602)*q+.0008620153809796321)*q-.0003577445606469842;
        da=fitted?((3*1.0800821774906936e-5*q-2*.00016848836814836602)*q+.0008620153809796321):0;
        threshold=.25610349392710086+.27051216771368214*q;
        dt=fitted?.27051216771368214:0; lambda=.06;
    }else{
        alpha=.020662094888127674-.0017181795239085821*q;
        da=fitted?-.0017181795239085821:0;
        threshold=1.208306917691355+.3139084341943607*q;
        dt=fitted?.3139084341943607:0; lambda=0;
    }
    const double over=u-threshold;
    if(over<=0)return {0,0,0};
    if(d<=over){
        const double base=(over-.5*d)*d, channel=1+lambda*d;
        return {alpha*base*channel,(da*base+alpha*(1-dt)*d)*channel,
                alpha*((over-d)*channel+lambda*base)};
    }
    const double base=.5*over*over;
    return {alpha*base*(1+lambda*d),(da*base+alpha*over*(1-dt))*(1+lambda*d),alpha*base*lambda};
}
inline Current inverter(double gate,double out,double rail) noexcept {
    const auto n=mos(gate,out,false),p=mos(rail-gate,rail-out,true);
    return {n.value-p.value,n.gateDerivative+p.gateDerivative,n.drainDerivative+p.drainDerivative};
}
} // namespace parable::dsp::cmos_detail
