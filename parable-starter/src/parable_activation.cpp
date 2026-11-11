#include "parable_activation.h"
#include <new>

namespace parable::activation {
Runtime::Runtime(float rate) noexcept
    : rate_(rate), bundles_{Bundle(rate),Bundle(rate),Bundle(rate),Bundle(rate)} {}
void Runtime::release(size_t i) noexcept {
    bundles_[i].document.reset(); // Control only; Engine storage stays in the pool.
    slots_[i].phase = Phase::Free;
}
Code Runtime::acknowledge(Token token) noexcept {
    if(!token) return Code::Ok;
    auto& slot=slots_[index(token)];
    if(slot.token!=token) return Code::ProtocolFault;
    // A stop can retire a just-consumed pending bundle between control's loads.
    // Its release ACK proves both consumption AND completion of the last read.
    if(slot.phase==Phase::Pending && controlPending_==token) controlPending_=0;
    else if(slot.phase!=Phase::Active && slot.phase!=Phase::Retired) return Code::ProtocolFault;
    slot.phase=Phase::Retired;
    return Code::Ok;
}
Code Runtime::synchronize() noexcept {
    if (controlPending_ && pending_.load(std::memory_order_acquire) == 0) {
        auto& slot = slots_[index(controlPending_)];
        if (slot.token != controlPending_ || slot.phase != Phase::Pending) return Code::ProtocolFault;
        slot.phase = Phase::Active;
        controlPending_ = 0;
    }
    return acknowledge(retired_.load(std::memory_order_acquire));
}
Result Runtime::stage(std::string_view text) {
    if (stop_.load(std::memory_order_acquire) && !stopped()) return {Code::Stopping};
    const auto sync = synchronize(); if (sync != Code::Ok) return {sync};
    if (staging_) return {Code::Busy};
    size_t i=0; while (i<kBundles && slots_[i].phase != Phase::Free) ++i;
    if (i==kBundles) return {Code::Busy};
    const Token token = detail::issue(nextGeneration_,i);
    if (!token) return {Code::GenerationExhausted};
    slots_[i]={Phase::Staging,token}; staging_=token;
    try {
        auto decoded = patch::Codec::decode(text);
        if (const auto* error=std::get_if<patch::Error>(&decoded)) {
            Result result{Code::InvalidDocument}; result.error=*error; discard(token); return result;
        }
        const auto& document=std::get<patch::ValidatedPatch>(decoded);
        auto prepared=preparation::prepare(document,rate_);
        if (const auto* error=std::get_if<preparation::Error>(&prepared)) {
            Result result{Code::PreparationFailed}; result.error=*error; discard(token); return result;
        }
        const auto status=bundles_[i].engine.load(std::get<Patch>(prepared));
        if (status != parable::Status::Ok) {
            discard(token); Result result{Code::EngineFailed}; result.nativeStatus=status; return result;
        }
        bundles_[i].document=document;
        bundles_[i].token=token;
        return {Code::Ok,token};
    } catch (const std::bad_alloc&) { discard(token); return {Code::AllocationFailed}; }
}
Result Runtime::publish(Token token) {
    if (!token || token!=staging_ || slots_[index(token)].phase!=Phase::Staging) return {Code::InvalidToken};
    if (stop_.load(std::memory_order_acquire) && !stopped()) return {Code::Stopping};
    if(pending_.load(std::memory_order_acquire)) return {Code::Busy};
    // Consumption may happen between earlier control observations and this load.
    // Synchronize AFTER observing empty, before replacing controlPending_. With
    // one control writer the mailbox cannot become nonempty in between.
    const auto sync=synchronize(); if(sync!=Code::Ok) return {sync};
    slots_[index(token)].phase=Phase::Pending;
    staging_=0; controlPending_=token;
    pending_.store(token,std::memory_order_release);
    return {Code::Ok,token};
}
Result Runtime::discard(Token token) {
    if (!token || token!=staging_ || slots_[index(token)].phase!=Phase::Staging) return {Code::InvalidToken};
    release(index(token)); staging_=0; return {};
}
Result Runtime::collect() {
    const auto sync=synchronize(); if(sync!=Code::Ok) return {sync};
    const Token token=retired_.load(std::memory_order_acquire);
    if (!token) return {};
    if(acknowledge(token)!=Code::Ok) return {Code::ProtocolFault};
    release(index(token));
    retired_.store(0,std::memory_order_release);
    return {Code::Ok,token};
}
Snapshot Runtime::inspect() {
    auto code=synchronize();
    const auto retired=retired_.load(std::memory_order_acquire);
    if(code==Code::Ok) code=acknowledge(retired);
    return {code,slots_,staging_,controlPending_,retired};
}
std::optional<patch::ValidatedPatch> Runtime::document(Token token) const {
    if (!token || slots_[index(token)].token!=token || slots_[index(token)].phase==Phase::Free) return {};
    return bundles_[index(token)].document;
}
Result Runtime::startAfterJoin() noexcept {
    if(!stopped() || !stop_.load(std::memory_order_acquire) || active_ || retired_.load(std::memory_order_acquire)) return {Code::NotStopped};
    stopped_.store(0,std::memory_order_release);
    stop_.store(0,std::memory_order_release);
    return {};
}
void Runtime::stopAfterJoin() noexcept {
    // External quiescence establishes happens-before with all audio reads/writes.
    stop_.store(1,std::memory_order_release);
    for(size_t i=0;i<kBundles;++i) release(i);
    staging_=controlPending_=active_=0;
    pending_.store(0,std::memory_order_release); retired_.store(0,std::memory_order_release);
    stopped_.store(1,std::memory_order_release);
}
} // namespace parable::activation
