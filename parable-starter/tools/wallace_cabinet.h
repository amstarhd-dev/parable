#pragma once
#include "FFTConvolver.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace wallace2 {
// Independent mono HOST specimen. Owner prepares/reprepares while stopped.
// No WAV I/O, resampling, normalization or graph dependency in this processor.
class Cabinet {
public:
    bool prepare(unsigned rate,const float* ir,size_t taps) {
        if(rate!=48000||!ir||!taps||taps>32768)return false;
        double bound=0;
        for(size_t i=0;i<taps;++i){if(!std::isfinite(ir[i]))return false;bound+=std::abs(ir[i]);}
        if(bound==0||bound>256)return false;
        prepared_=false;
        prepared_=convolver_.init(64,ir,taps);return prepared_;
    }
    // Rejected blocks do not advance convolution history. All work bounded by
    // 64 frames/32768 taps. Input/output must be distinct valid arrays.
    bool process(const float* in,float* out,size_t frames=64) noexcept {
        if(!out||!frames||frames>64)return false;
        bool valid=prepared_&&in&&in!=out;
        if(valid)for(size_t i=0;i<frames;++i)valid&=std::isfinite(in[i])&&std::abs(in[i])<=16;
        if(!valid){std::fill_n(out,frames,0);return false;}
        convolver_.process(in,out,frames);return true;
    }
private:
    fftconvolver::FFTConvolver convolver_;
    bool prepared_=false;
};
}
