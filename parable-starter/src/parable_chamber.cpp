#include "parable_chamber.h"
#include "chamber_cloud_settings.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
namespace parable::soundforge {
struct Chamber::Core:Cloudseed::ReverbController {explicit Core(int rate):ReverbController(rate){}};
Chamber::Chamber()=default;Chamber::~Chamber()=default;
std::size_t Chamber::coreBytes()noexcept{return sizeof(Core);}
bool Chamber::prepare(float rate,float gain,float wet){
    if((rate!=44100&&rate!=48000&&rate!=96000)||!std::isfinite(gain)||gain<0||gain>64||!std::isfinite(wet)||wet<0||wet>1)return false;
    // Upstream uses process-global rand during construction: serialized control
    // preparation only. Never reseed/allocate on the audio thread.
    std::srand(1);auto fresh=std::make_unique<Core>(static_cast<int>(rate));
    chamber_detail::compact(*fresh);core_=std::move(fresh);gain_=targetGain_=gain;wet_=wet;
    gainPole_=1-std::exp(-1/(.02f*rate));
    hp_=lp_=prev_=0;hpPole_=std::exp(-6.28318530718f*200/rate);lpPole_=std::exp(-6.28318530718f*4500/rate);
    for(unsigned i=0;i<64;++i)memory_[i]=mic_[i]=speaker_[i]=applied_[i]=0;
    outputClamps_=0;return true;
}
float Chamber::returnAmplifier(float microphone,float gain)noexcept{
    // Common biased complementary conduction law + common soft voltage rail.
    // Gain corresponds to fixed feedback resistance / one isolation resistance.
    const float conduct=std::copysign(std::max(std::abs(microphone)-.007f,0.f),microphone);
    return .15f*std::tanh(gain*conduct/.15f);
}
bool Chamber::setReturnGain(float gain)noexcept{
    if(!std::isfinite(gain)||gain<0||gain>64)return false;
    targetGain_=gain;return true;
}
bool Chamber::process(const float* input,float* output)noexcept{
    if(!core_){std::fill_n(output,64,0.f);return false;}
    for(unsigned i=0;i<64;++i)if(!std::isfinite(input[i])){std::fill_n(output,64,0.f);return false;}
    float excitation[64],left[64],right[64];
    for(unsigned i=0;i<64;++i){applied_[i]=memory_[i];excitation[i]=speaker_[i]=std::clamp(input[i],-1.f,1.f)+memory_[i];}
    core_->Process(excitation,excitation,left,right,64);
    for(unsigned i=0;i<64;++i){
        mic_[i]=.5f*(left[i]+right[i]);
        hp_=hpPole_*(hp_+mic_[i]-prev_);prev_=mic_[i];
        lp_+=(1-lpPole_)*(hp_-lp_);
        if(std::abs(lp_)<1.e-25f)lp_=0;
        if(std::abs(hp_)<1.e-25f)hp_=0;
        gain_+=gainPole_*(targetGain_-gain_);
        memory_[i]=returnAmplifier(lp_,gain_); // used64samples later, at actual excitation
        // Fixed display/wet-return calibration for BOTH states, not normalization.
        const float raw=(1-wet_)*std::clamp(input[i],-1.f,1.f)+wet_*3.f*mic_[i];
        output[i]=std::clamp(raw,-.98f,.98f);if(output[i]!=raw)++outputClamps_;
    }
    return true;
}
}
