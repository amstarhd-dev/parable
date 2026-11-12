#include "parable_living.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace parable::living {
namespace {
bool clockValid(const Clock& c) noexcept {
    return c.timebase && c.epoch && c.horizon && c.horizon<=kHorizon &&
           c.sourceAnchor<=UINT64_MAX-c.horizon && c.controlAnchor<=UINT64_MAX-2*c.horizon &&
           c.uncertainty<=c.horizon && std::isfinite(c.scale) && c.scale>=.99 && c.scale<=1.01;
}
bool units(Kind k,Unit u) noexcept {
    switch(k) {
    case Kind::HeartRate: return u==Unit::Bpm;
    case Kind::Interval: case Kind::HrvSummary: return u==Unit::Millisecond;
    case Kind::Ppg: return u==Unit::Arbitrary;
    case Kind::Ecg: return u==Unit::Microvolt;
    case Kind::Respiration: return u==Unit::BreathsPerMinute;
    case Kind::Motion: return u==Unit::MilliG;
    case Kind::Eda: return u==Unit::Microsiemens;
    case Kind::Temperature: return u==Unit::Celsius;
    case Kind::Derived: return u==Unit::Normalized || u==Unit::Arbitrary;
    case Kind::Control: return u==Unit::Normalized;
    }
    return false;
}
bool values(const Record& r) noexcept {
    for(float x:r.value) if(!std::isfinite(x) || std::abs(x)>1e9f) return false;
    if(r.kind!=Kind::Motion && (r.value[1]!=0 || r.value[2]!=0)) return false;
    if(r.validity!=Validity::Valid) return true; // Missing has no measured value.
    const float x=r.value[0];
    if(r.unit==Unit::Normalized) return x>=0 && x<=1;
    switch(r.kind) {
    case Kind::HeartRate: return x>0 && x<=400;
    case Kind::Interval: return x>0 && x<=60000;
    case Kind::HrvSummary: return x>=0 && x<=60000;
    case Kind::Respiration: return x>=0 && x<=200;
    case Kind::Eda: return x>=0 && x<=100000;
    case Kind::Temperature: return x>=-273.15f && x<=1000;
    default: return true;
    }
}
} // namespace
Code Lane::publish(Snapshot s) noexcept {
    if(epoch_.load(std::memory_order_relaxed)==UINT32_MAX) return Code::Exhausted;
    if(published_ && s.at<=lastPublished_) return Code::Order;
    for(size_t i=0;i<2;++i) {
        if(!std::isfinite(s.normalized[i]) || s.normalized[i]<0 || s.normalized[i]>1 ||
           static_cast<unsigned>(s.state[i])>static_cast<unsigned>(State::ClockReset) ||
           (s.state[i]==State::Fresh && (s.until[i]<s.at || s.until[i]-s.at>kHorizon))) return Code::Invalid;
    }
    const uint32_t w=written_.load(std::memory_order_relaxed);
    const uint32_t r=read_.load(std::memory_order_acquire);
    if(w-r==kSnapshots) return Code::Full;
    // Monotonic counters deliberately refuse wrap instead of ambiguous old sessions.
    if(w==UINT32_MAX) return Code::Exhausted;
    s.epoch=epoch_.load(std::memory_order_relaxed);
    queue_[w%kSnapshots]=s;
    written_.store(w+1,std::memory_order_release);
    lastPublished_=s.at; published_=true;
    return Code::Ok;
}
Code Lane::invalidate() noexcept {
    const uint32_t e=epoch_.load(std::memory_order_relaxed);
    if(e==UINT32_MAX) return Code::Exhausted;
    epoch_.store(e+1,std::memory_order_release);
    return Code::Ok;
}
void Lane::resetAfterJoin() noexcept {
    written_.store(0,std::memory_order_release); read_.store(0,std::memory_order_release);
    lastPublished_=0; published_=false;
    // Existing snapshots are gone; exhaustion does not grant a fresh identity.
}
size_t Scheduler::find(uint32_t s,uint32_t c) const noexcept {
    for(size_t i=0;i<channels_;++i) if(channel_[i].profile.source==s && channel_[i].profile.channel==c) return i;
    return kChannels;
}
Code Scheduler::declare(const Profile& p) noexcept {
    if(!p.source || !p.channel || !units(p.kind,p.unit) || !clockValid(p.clock) || !p.ttl || p.ttl>kHorizon ||
       p.clock.uncertainty>p.ttl || p.consentUntil==UINT64_MAX ||
       static_cast<unsigned>(p.origin)>1 || static_cast<unsigned>(p.delivery)>1 ||
       static_cast<unsigned>(p.permission)>3 || static_cast<unsigned>(p.destination)>2 || static_cast<unsigned>(p.background)>2 ||
       !std::isfinite(p.acquisitionHz) || !std::isfinite(p.deliveryHz) ||
       p.acquisitionHz<0 || p.acquisitionHz>100000 || p.deliveryHz<0 || p.deliveryHz>100000 ||
       (p.destination!=Destination::None && p.kind!=Kind::Control)) return Code::Invalid;
    if(channels_==kChannels) return Code::Full;
    if(find(p.source,p.channel)!=kChannels) return Code::Conflict;
    for(size_t i=0;i<channels_;++i) {
        const auto& old=channel_[i].profile;
        if(p.destination!=Destination::None && old.destination==p.destination) return Code::Conflict;
        if(old.source==p.source && (old.clock.timebase!=p.clock.timebase || old.clock.epoch!=p.clock.epoch ||
           old.clock.sourceAnchor!=p.clock.sourceAnchor || old.clock.controlAnchor!=p.clock.controlAnchor ||
           old.clock.scale!=p.clock.scale)) return Code::Clock;
    }
    channel_[channels_++].profile=p;
    return Code::Ok;
}
void Scheduler::flush(size_t slot) noexcept {
    size_t kept=0;
    for(size_t i=0;i<queued_;++i) {
        const size_t from=(head_+i)%kRecords;
        if(queue_[from].slot!=slot) queue_[(head_+kept++)%kRecords]=queue_[from];
    }
    queued_=kept;
}
Code Scheduler::permission(uint32_t source,uint32_t channel,Permission p,Time until) noexcept {
    const auto i=find(source,channel);
    if(i==kChannels) return Code::UnknownChannel;
    if(static_cast<unsigned>(p)>3 || (p==Permission::Granted && (until<=now_ || until==UINT64_MAX))) return Code::Invalid;
    // Every permission transition invalidates queued audio controls before any state changes.
    const auto code=lane_.invalidate(); if(code!=Code::Ok) return code;
    auto& c=channel_[i]; c.profile.permission=p; c.profile.consentUntil=until;
    c.view.present=c.view.ageKnown=false; c.view.age=0;
    c.view.state=p==Permission::Granted?State::Missing:State::NoPermission; flush(i);
    // Preserve sequence watermark: regrant alone cannot authorize old/replayed packets.
    return Code::Ok;
}
Code Scheduler::resetClock(uint32_t source,const Clock& clock) noexcept {
    if(!clockValid(clock)) return Code::Invalid;
    bool found=false;
    for(size_t i=0;i<channels_;++i) if(channel_[i].profile.source==source) {
        found=true; if(clock.epoch<=channel_[i].profile.clock.epoch || clock.uncertainty>channel_[i].profile.ttl) return Code::Clock;
    }
    if(!found) return Code::UnknownChannel;
    const auto result=lane_.invalidate(); if(result!=Code::Ok) return result;
    for(size_t i=0;i<channels_;++i) if(channel_[i].profile.source==source) {
        auto& c=channel_[i]; c.profile.clock=clock; c.ordered=c.measured=false;
        c.view.present=c.view.ageKnown=false; c.view.age=0; c.view.state=State::ClockReset; flush(i);
    }
    return Code::Ok;
}
Code Scheduler::ingest(const Record& r) noexcept {
    const auto reject=[&](Code code){++counts_.rejected; return code;};
    const auto i=find(r.source,r.channel); if(i==kChannels) return reject(Code::UnknownChannel);
    auto& c=channel_[i]; const auto& p=c.profile;
    if(r.kind!=p.kind || r.unit!=p.unit || r.origin!=p.origin || r.delivery!=p.delivery ||
       static_cast<unsigned>(r.validity)>2 || static_cast<unsigned>(r.quality)>2 ||
       static_cast<unsigned>(r.permission)>3 || (r.flags&~3u) || !r.sequence || !values(r) ||
       !std::isfinite(r.confidence) || ((r.flags&ConfidenceKnown) && (r.confidence<0 || r.confidence>1))) return reject(Code::Invalid);
    if(p.permission!=Permission::Granted || r.permission!=Permission::Granted || r.arrived>=p.consentUntil) return reject(Code::NoPermission);
    if(r.timebase!=p.clock.timebase || r.epoch!=p.clock.epoch) return reject(Code::Clock);
    if(advanced_ && r.arrived<=now_) return reject(Code::Late);
    if(ingested_ && r.arrived<lastArrival_) return reject(Code::Order);
    if(c.ordered && r.sequence<=c.sequence) return reject(r.sequence==c.sequence?Code::Duplicate:Code::Order);
    Time mapped=0;
    if(r.flags&MeasurementKnown) {
        if(c.measured && r.measured<c.lastMeasured) return reject(Code::Order);
        if(r.measured<p.clock.sourceAnchor || r.measured-p.clock.sourceAnchor>p.clock.horizon) return reject(Code::Clock);
        const auto delta=static_cast<Time>(std::llround(double(r.measured-p.clock.sourceAnchor)*p.clock.scale));
        mapped=p.clock.controlAnchor+delta; // bounded by validated horizon/anchors.
        if(mapped>r.arrived || r.arrived-mapped>p.ttl || p.clock.uncertainty>p.ttl-(r.arrived-mapped)) return reject(Code::Stale);
        if(mapped>UINT64_MAX-p.ttl) return reject(Code::Clock);
    }
    if(queued_==kRecords) {++counts_.overflow; return reject(Code::Full);}
    queue_[(head_+queued_)%kRecords]={r,mapped,i}; ++queued_;
    counts_.highWater=std::max(counts_.highWater,queued_); ++counts_.accepted;
    if(c.ordered && r.sequence-c.sequence>1) c.view.gaps+=r.sequence-c.sequence-1;
    c.sequence=r.sequence; c.lastArrival=r.arrived; c.ordered=true;
    if(r.flags&MeasurementKnown) {c.lastMeasured=r.measured; c.measured=true;}
    lastArrival_=r.arrived; ingested_=true;
    return Code::Ok;
}
Code Scheduler::advance(Time now,Observer observer,void* context) noexcept {
    if(advanced_ && now<=now_) return Code::Order;
    now_=now; advanced_=true;
    // Single control writer: expiry invalidates the lane even if its FIFO is full.
    for(size_t i=0;i<channels_;++i) {
        auto& c=channel_[i];
        if(c.profile.permission==Permission::Granted && now>=c.profile.consentUntil) {
            const auto code=permission(c.profile.source,c.profile.channel,Permission::Expired,now);
            if(code!=Code::Ok) return code;
        }
    }
    for(size_t n=0;n<kRecords && queued_;++n) {
        const auto& item=queue_[head_]; if(item.record.arrived>now) break;
        auto& v=channel_[item.slot].view;
        v.last=item.record; v.present=true; v.mapped=item.mapped; ++v.accepted;
        if(observer) observer(context,item.record,item.mapped);
        head_=(head_+1)%kRecords; --queued_;
    }
    Snapshot snapshot; snapshot.at=now;
    for(size_t i=0;i<channels_;++i) {
        auto& c=channel_[i]; auto& v=c.view;
        v.ageKnown=v.present && (v.last.flags&MeasurementKnown);
        v.age=v.ageKnown && now>=v.mapped ? now-v.mapped : 0;
        if(c.profile.permission!=Permission::Granted) v.state=State::NoPermission;
        else if(v.present) {
            if(v.last.validity==Validity::Missing) v.state=State::Missing;
            else if(v.last.validity!=Validity::Valid || v.last.quality==Quality::Poor) v.state=State::Invalid;
            else if(!v.ageKnown || v.age>=c.profile.ttl-c.profile.clock.uncertainty) v.state=State::Stale;
            else v.state=State::Fresh;
        }
        if(c.profile.destination!=Destination::None) {
            const auto d=c.profile.destination==Destination::PreGain?0u:1u;
            snapshot.state[d]=v.state;
            // Only a normalized Control with explicitly good quality may modulate.
            if(v.state==State::Fresh && v.last.quality==Quality::Good) {
                snapshot.normalized[d]=v.last.value[0];
                snapshot.until[d]=std::min(v.mapped+c.profile.ttl-c.profile.clock.uncertainty,c.profile.consentUntil);
            } else if(v.state==State::Fresh) snapshot.state[d]=State::Invalid;
        }
    }
    const auto code=lane_.publish(snapshot); if(code==Code::Full) ++counts_.snapshotFull;
    return code;
}
View Scheduler::view(size_t i) const noexcept {return i<channels_?channel_[i].view:View{};}
} // namespace parable::living
