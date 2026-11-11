#include "parable_activation.h"
namespace parable::activation {
Receipt Runtime::processBlock(const float* input,float* output,size_t frames) noexcept {
    if(frames>kMaxBlockFrames || (frames && (!input || !output))) return {BlockCode::InvalidBlock,active_,0,false};
    BlockCode code=BlockCode::Rendered; bool adopted=false;
    if(stop_.load(std::memory_order_acquire)) {
        if(active_ && retired_.load(std::memory_order_acquire)) code=BlockCode::StopDeferred;
        else {
            if(active_) { retired_.store(active_,std::memory_order_release); active_=0; }
            stopped_.store(1,std::memory_order_release);
            for(size_t n=0;n<frames;++n) output[n]=0;
            return {BlockCode::Stopped,0,frames,false};
        }
    } else {
        const Token next=pending_.load(std::memory_order_acquire);
        if(next) {
            if(retired_.load(std::memory_order_acquire)) code=BlockCode::Deferred;
            else if(bundles_[index(next)].token!=next) code=BlockCode::ProtocolFault;
            else {
                const Token old=active_; active_=next; adopted=true;
                // No old history is read after this release (last read was prior block).
                if(old) retired_.store(old,std::memory_order_release);
                pending_.store(0,std::memory_order_release); // Consumption ACK, after retirement publication.
            }
        }
    }
    const Token token=active_; // Stable for this ENTIRE block, even if control publishes again.
    if(token) {
        auto& engine=bundles_[index(token)].engine;
        for(size_t n=0;n<frames;++n) output[n]=engine.process(input[n]);
    } else {
        code=BlockCode::NoActive;
        for(size_t n=0;n<frames;++n) output[n]=0;
    }
    return {code,token,frames,adopted};
}
} // namespace parable::activation
