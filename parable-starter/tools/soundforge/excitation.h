#pragma once
// Owned neutral excitation extracted from PAR-007; not a recorded guitar model.
#include "../lab_wav.h"
#include "analysis.h"
namespace soundforge {
inline lab::Wave syntheticPlucks(double peak=.15) {
    require(std::isfinite(peak)&&peak>0&&peak<1,"Invalid synthetic peak");
    constexpr double pi=3.14159265358979323846;
    lab::Wave w; w.samples.resize(w.rate*9);
    const double strings[] = {82.4069,110,146.8324,195.9977,246.9417,329.6276};
    for (std::size_t i = 0; i < w.samples.size(); ++i) {
        const double t = double(i)/w.rate; double x = 0;
        for (int event = 0; event < 5; ++event) {
            const bool strum = event >= 2; const double start = .25+event*1.55;
            for (int string = 0; string < (strum ? 6 : 1); ++string) {
                const double age = t-start-string*.012;
                if (age < 0 || age > 2) continue;
                const double f = strings[string] * (event==1 ? 1.5 : 1.);
                const double release = age <= 1.9 ? 1. : .5*(1+std::cos(pi*(age-1.9)/.1));
                for (int harmonic=1; harmonic<=10; ++harmonic)
                    x += release * (event%2 ? .55 : 1.) * (1-std::exp(-age*600))
                         * std::exp(-age*(2.2+.26*harmonic)) / std::pow(harmonic,1.3)
                         * std::sin(2*pi*f*harmonic*age+.13*harmonic*harmonic);
            }
        }
        w.samples[i] = static_cast<float>(x);
    }
    const auto s = stats(w.samples); for(auto& x:w.samples)x=static_cast<float>(x*peak/s.peak);
    return w;
}
} // namespace soundforge
