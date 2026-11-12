#include "parable_living.h"
#include <cmath>

namespace parable::living {
Audio::Audio(Lane& lane,activation::Runtime& runtime,uint32_t rate) noexcept
    : lane_(lane), runtime_(runtime) {
    // 20 ms maximum full-scale gain traversal. Invalid rates disable modulation.
    if(rate>=8000 && rate<=192000) step_=2.f/(.02f*static_cast<float>(rate));
    resetAfterJoin();
}
void Audio::resetAfterJoin() noexcept {
    target_=Snapshot{}; gain_={1,1}; epoch_=lane_.epoch(); timed_=false; last_=0;
}
AudioReceipt Audio::processBlock(const float* input,float* output,size_t frames,Time now) noexcept {
    AudioReceipt receipt{}; receipt.epoch=epoch_;
    if(frames>activation::kMaxBlockFrames || (frames && (!input || !output))) {
        receipt.patch={activation::BlockCode::InvalidBlock,0,0,false}; return receipt;
    }
    const bool timeError=timed_ && (now<last_ || (frames && now==last_));
    if(!timeError) {last_=now; timed_=true;}
    const auto epoch=lane_.epoch_.load(std::memory_order_acquire);
    if(epoch!=epoch_ || timeError) {target_=Snapshot{}; epoch_=epoch;}
    const auto written=lane_.written_.load(std::memory_order_acquire);
    auto read=lane_.read_.load(std::memory_order_relaxed);
    for(size_t n=0;n<kSnapshots && read!=written;++n) {
        const auto& next=lane_.queue_[read%kSnapshots];
        if(next.epoch>epoch) break; // producer invalidated after this boundary's epoch load.
        if(next.epoch==epoch && next.at>now && !timeError) break;
        if(next.epoch==epoch && !timeError) target_=next;
        ++read; ++receipt.consumed;
    }
    lane_.read_.store(read,std::memory_order_release); // final read of reusable slots.
    std::array<float,2> goal{1,1};
    for(size_t d=0;d<2;++d) {
        receipt.state[d]=target_.state[d];
        if(timeError || epoch==UINT32_MAX) receipt.state[d]=State::ClockReset;
        else if(target_.state[d]==State::Fresh && now<target_.until[d] && step_>0)
            goal[d]=target_.normalized[d]*(d==0?2.f:1.f);
        else if(target_.state[d]==State::Fresh) receipt.state[d]=State::Stale;
    }
    for(size_t i=0;i<frames;++i) {
        for(size_t d=0;d<2;++d) {
            const float delta=goal[d]-gain_[d];
            gain_[d]+=delta>step_?step_:(delta< -step_? -step_:delta);
        }
        // Preserve nonfinite input rejection in Engine. Bound BEFORE multiplication.
        const float x=input[i];
        scratch_[i]=!std::isfinite(x)?x:(x>256?256:(x< -256? -256:x));
        scratch_[i]*=gain_[0]; post_[i]=gain_[1];
    }
    receipt.patch=runtime_.processBlock(scratch_.data(),output,frames); // one identity, once/block.
    for(size_t i=0;i<frames;++i) output[i]*=post_[i];
    receipt.target=target_; receipt.gain=gain_; receipt.epoch=epoch_; receipt.timeError=timeError;
    return receipt;
}
} // namespace parable::living
