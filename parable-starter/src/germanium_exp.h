#pragma once
// Private numerical experiment; no public transistor/processor API or state.
#include "germanium_exp_tables.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
namespace parable::dsp::germanium_detail {
struct Exponential {double value,derivative;};
#ifdef PARABLE_GERMANIUM_COUNT_APPROX
inline std::uint64_t approximationCalls=0;
#endif
struct Reduction {int exponent;double remainder;};
inline Reduction reduceExponential(double z) noexcept {
    constexpr double invLn2=0x1.71547652b82fep+0,ln2Hi=0x1.62e42fee00000p-1,ln2Lo=0x1.a39ef35793c76p-33;
    const int k=static_cast<int>(z*invLn2+(z<0?-.5:.5));
    return {k,(z-k*ln2Hi)-k*ln2Lo};
}
inline double binaryScale(int k) noexcept {
    static_assert(sizeof(double)==sizeof(std::uint64_t)&&std::numeric_limits<double>::is_iec559,"Requires IEEE binary64");
    const std::uint64_t bits=std::uint64_t(k+1023)<<52;double value;std::memcpy(&value,&bits,sizeof(value));return value;
}
template<unsigned Cells> struct HermiteReduced {
    static_assert(Cells==32||Cells==64,"Only audited table sizes");
    static Exponential evaluate(double a) noexcept {
        constexpr double h=Cells==32?tables::step32:tables::step64;
        if(a==0)return {a,1}; // preserve the sign of expm1(-0)
        const bool negative=std::signbit(a);
        unsigned i=static_cast<unsigned>(std::abs(a)*(1/h));if(i>=Cells)i=Cells-1;
        const tables::Cubic* cells;
        if constexpr(Cells==32)cells=negative?tables::hermite32negative.data():tables::hermite32.data();
        else cells=negative?tables::hermite64negative.data():tables::hermite64.data();
        const auto& c=cells[i]; // explicit pointer into static readonly storage
        const double d=a-(negative?-double(i):double(i))*h;
        return {c.a0+d*(c.a1+d*(c.a2+d*c.a3)),c.a1+d*(2*c.a2+3*d*c.a3)};
    }
};
template<unsigned Degree> struct PolynomialReduced {
    static_assert(Degree==8||Degree==12,"Only audited polynomial degrees");
    static Exponential evaluate(double a) noexcept {
        const auto& c=[]()->const auto&{if constexpr(Degree==8)return tables::poly8;else return tables::poly12;}();
        double value=c[Degree],derivative=0;
        for(int i=Degree-1;i>=1;--i){derivative=derivative*a+value;value=value*a+c[unsigned(i)];}
        return {a*value,a*derivative+value};
    }
};
template<class Positive> struct FastExponential {
    static Exponential evaluate(double z) noexcept {
#ifdef PARABLE_GERMANIUM_COUNT_APPROX
        ++approximationCalls;
#endif
        if(!(z>=-416&&z<=28)){const double q=std::numeric_limits<double>::quiet_NaN();return {q,q};}
        if(z<=-40)return {-1,0};
        const auto r=reduceExponential(z);const auto p=Positive::evaluate(r.remainder);
        const double scale=binaryScale(r.exponent);
        if(z< -36){const double t=(z+40)/4,w=t*t*(3-2*t),wdot=1.5*t*(1-t);
            const double e=scale*(1+p.value);return {w*e-1,w*scale*p.derivative+wdot*e};}
        if(r.exponent==0)return p; // cancellation/sign-preserving near zero
        return {scale*p.value+(scale-1),scale*p.derivative};
    }
};
using Hermite32=FastExponential<HermiteReduced<32>>;
using Hermite64=FastExponential<HermiteReduced<64>>;
using Polynomial8=FastExponential<PolynomialReduced<8>>;
using Polynomial12=FastExponential<PolynomialReduced<12>>;
} // namespace parable::dsp::germanium_detail
