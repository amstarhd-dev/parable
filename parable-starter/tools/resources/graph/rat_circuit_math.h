#pragma once
// Feedback polynomial adapted from PiRAT TFGain.cpp (Cedric TESSIER, BSD-2-Clause),
// ported from Rodent.V2/TFGain.cs (Valdemar Erlingsson, MIT).
// Full notices and exact revisions: third_party/labrat/ and source audit.
// Changes: explicit components/open leg, finite-A extension, fixed arrays/doubles.
// Bilinear below independently expands the standard substitution s=2fs(1-z^-1)/(1+z^-1);
// no AudioLib implementation is copied (no source license found there).
#include <array>
#include <cmath>
namespace parable::dsp::rat_detail {
using Polynomial = std::array<double,5>;
inline void feedbackPolynomial(double r1,double r2,double c1,double c2,double gain,double c3,
                               Polynomial& b,Polynomial& a) noexcept {
    b = {1., c1*gain+c2*gain+c3*gain+c1*r1+c2*r2,
        c1*c2*gain*r1+c1*c3*gain*r1+c1*c2*gain*r2+c2*c3*gain*r2+c1*c2*r1*r2,
        c1*c2*c3*gain*r1*r2,0};
    a = {1.,c3*gain+c1*r1+c2*r2,
        c1*c3*gain*r1+c2*c3*gain*r2+c1*c2*r1*r2,c1*c2*c3*gain*r1*r2,0};
}
inline unsigned choose(unsigned n,unsigned k) noexcept {
    unsigned v=1;for(unsigned i=1;i<=k;++i)v=v*(n-i+1)/i;return v;
}
inline void bilinear(const Polynomial& b,const Polynomial& a,unsigned order,double rate,
                     Polynomial& zb,Polynomial& za) noexcept {
    zb.fill(0);za.fill(0);
    for(unsigned j=0;j<=order;++j) {
        const double scale=std::pow(2*rate,static_cast<int>(j));
        for(unsigned p=0;p<=j;++p)for(unsigned q=0;q<=order-j;++q) {
            const double weight=double(choose(j,p))*choose(order-j,q)*(p%2 ? -1.:1.)*scale;
            zb[p+q]+=b[j]*weight;za[p+q]+=a[j]*weight;
        }
    }
    const double norm=za[0];for(unsigned i=0;i<=order;++i){zb[i]/=norm;za[i]/=norm;}
}
inline double pot(double x) noexcept {return (std::pow(20.,x)-1)/19.;}
} // namespace parable::dsp::rat_detail

