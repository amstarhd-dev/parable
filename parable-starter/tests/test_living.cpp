#include "living_fixture.h"
#include "activation_guard.h"
#include "test_check.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <thread>

namespace l=parable::living;
namespace a=parable::activation;
namespace f=living_fixture;
namespace g=activation_guard;
#define VERIFY(x) do {if(!(x)){std::cerr<<"living failure "<<__LINE__<<": "<<#x<<'\n';std::abort();}}while(false)
static std::string document(const char* name="rat-composite.json") {
    std::ifstream file(std::string(PATCH_FIXTURES)+"/"+name);CHECK(bool(file));return {std::istreambuf_iterator<char>(file),{}};
}
static parable::Patch native(const std::string& doc) {
    const auto decoded=parable::patch::Codec::decode(doc);CHECK(std::holds_alternative<parable::patch::ValidatedPatch>(decoded));
    const auto prepared=parable::preparation::prepare(std::get<parable::patch::ValidatedPatch>(decoded),48000);
    CHECK(std::holds_alternative<parable::Patch>(prepared));return std::get<parable::Patch>(prepared);
}
struct Rig {
    a::Runtime runtime{48000};l::Lane lane; l::Scheduler scheduler{lane};l::Audio audio{lane,runtime,48000};
    a::Token start(const std::string& doc) {
        const auto ready=runtime.stage(doc);CHECK(ready.code==a::Code::Ok);
        CHECK(runtime.publish(ready.token).code==a::Code::Ok);CHECK(runtime.startAfterJoin().code==a::Code::Ok);return ready.token;
    }
    ~Rig(){runtime.stopAfterJoin();}
};
static l::AudioReceipt guarded(Rig& r,const float* in,float* out,size_t frames,l::Time now) {
    g::begin();const auto receipt=r.audio.processBlock(in,out,frames,now);g::end();VERIFY(g::clean());return receipt;
}
static void drain(Rig& r,l::Time now) {guarded(r,nullptr,nullptr,0,now);}
static l::Record sample(size_t profile,l::Time t,uint64_t n,float x) {return f::record(f::profiles()[profile],t,n,x);}
static void contract() {
    g::selfTest();auto owned=std::make_unique<Rig>();auto& r=*owned;auto p=f::profiles();
    for(const auto& profile:p) CHECK(r.scheduler.declare(profile)==l::Code::Ok);
    CHECK(r.scheduler.channels()==15);CHECK(r.scheduler.declare(p[0])==l::Code::Conflict);
    auto bad=p[0];bad.source=99;bad.unit=l::Unit::Normalized;CHECK(r.scheduler.declare(bad)==l::Code::Invalid);
    bad=p[13];bad.source=99;CHECK(r.scheduler.declare(bad)==l::Code::Conflict);
    bad=p[0];bad.channel=2;bad.clock.scale=1.001;CHECK(r.scheduler.declare(bad)==l::Code::Clock);
    bad=p[0];bad.source=99;bad.destination=l::Destination::None;CHECK(r.scheduler.declare(bad)==l::Code::Ok);
    bad.source=98;CHECK(r.scheduler.declare(bad)==l::Code::Full);
    CHECK(r.scheduler.advance(0)==l::Code::Ok);CHECK(r.scheduler.view(13).state==l::State::Missing);drain(r,0);
    auto rec=sample(13,1,1,.25f);CHECK(r.scheduler.ingest(rec)==l::Code::Ok);
    auto duplicate=rec;CHECK(r.scheduler.ingest(duplicate)==l::Code::Duplicate);
    CHECK(r.scheduler.advance(1)==l::Code::Ok);CHECK(r.scheduler.view(13).state==l::State::Fresh);drain(r,1);
    CHECK(r.scheduler.ingest(rec)==l::Code::Late);CHECK(r.scheduler.advance(1)==l::Code::Order);
    rec=sample(13,2,2,.8f);
    for(float x:{NAN,INFINITY,-INFINITY,1e30f,-1.f,1.01f}) {
        auto b=rec;b.value[0]=x;CHECK(r.scheduler.ingest(b)==l::Code::Invalid);
    }
    for(auto mutate:{0,1,2,3,4,5,6,7,8,9}) {
        auto b=rec;
        switch(mutate) {
        case 0:b.unit=l::Unit::Bpm;break;case 1:b.kind=l::Kind::Interval;break;
        case 2:b.value[1]=1;break;case 3:b.flags=255;break;case 4:b.confidence=NAN;break;
        case 5:b.confidence=2;break;case 6:b.validity=static_cast<l::Validity>(99);break;
        case 7:b.quality=static_cast<l::Quality>(99);break;case 8:b.origin=l::Origin::Observed;break;
        case 9:b.sequence=0;break;
        }
        CHECK(r.scheduler.ingest(b)==l::Code::Invalid);
    }
    rec.epoch=2;CHECK(r.scheduler.ingest(rec)==l::Code::Clock);rec.epoch=1;
    rec.source=999;CHECK(r.scheduler.ingest(rec)==l::Code::UnknownChannel);rec.source=14;
    rec.permission=l::Permission::Revoked;CHECK(r.scheduler.ingest(rec)==l::Code::NoPermission);rec.permission=l::Permission::Granted;
    CHECK(r.scheduler.ingest(rec)==l::Code::Ok);CHECK(r.scheduler.advance(10000)==l::Code::Ok);drain(r,10000);
    CHECK(r.scheduler.view(13).last.value[0]==.8f);
    CHECK(r.scheduler.permission(14,1,l::Permission::Revoked,0)==l::Code::Ok);
    rec=sample(13,20000,3,.1f);CHECK(r.scheduler.ingest(rec)==l::Code::NoPermission);
    CHECK(r.scheduler.advance(20000)==l::Code::Ok);drain(r,20000);CHECK(r.scheduler.view(13).state==l::State::NoPermission);
    CHECK(r.scheduler.permission(14,1,l::Permission::Granted,30000)==l::Code::Ok);
    CHECK(r.scheduler.view(13).state==l::State::Missing && !r.scheduler.view(13).ageKnown);
    CHECK(r.scheduler.ingest(rec)==l::Code::Late); // permission does not reset committed time.
    rec=sample(13,20001,3,.1f);CHECK(r.scheduler.ingest(rec)==l::Code::Ok);
    CHECK(r.scheduler.advance(30000)==l::Code::Ok);drain(r,30000);CHECK(r.scheduler.view(13).state==l::State::NoPermission);
    CHECK(!r.scheduler.view(13).present);CHECK(r.scheduler.queued()==0);
    CHECK(r.scheduler.permission(999,1,l::Permission::Granted,40000)==l::Code::UnknownChannel);
    CHECK(r.scheduler.permission(14,1,l::Permission::Granted,30000)==l::Code::Invalid);
    // Five-minute-old values are not made fresh by their arrival; unknown timestamp remains unknown.
    rec=sample(1,0,1,70);rec.arrived=300000001;CHECK(r.scheduler.ingest(rec)==l::Code::Stale);
    rec.flags=0;CHECK(r.scheduler.ingest(rec)==l::Code::Ok);CHECK(r.scheduler.advance(rec.arrived)==l::Code::Ok);drain(r,rec.arrived);
    CHECK(r.scheduler.view(1).present && !r.scheduler.view(1).ageKnown && r.scheduler.view(1).state==l::State::Stale);
    std::cout<<"layout record="<<sizeof(l::Record)<<" profile="<<sizeof(l::Profile)<<" view="<<sizeof(l::View)
             <<" snapshot="<<sizeof(l::Snapshot)<<" lane="<<sizeof(l::Lane)<<" scheduler="<<sizeof(l::Scheduler)<<" audio="<<sizeof(l::Audio)<<" runtime="<<sizeof(a::Runtime)<<'\n';
}
static void timing() {
    auto owned=std::make_unique<Rig>();auto& r=*owned;auto p=f::profiles();
    p[2].ttl=6000000;p[13].clock.scale=1.0001;p[13].clock.uncertainty=10;
    for(size_t i:{2u,13u})CHECK(r.scheduler.declare(p[i])==l::Code::Ok);
    auto rec=f::record(p[13],1000000,1,.2f);rec.arrived=1000110;
    CHECK(r.scheduler.ingest(rec)==l::Code::Ok);CHECK(r.scheduler.advance(rec.arrived)==l::Code::Ok);drain(r,rec.arrived);
    CHECK(r.scheduler.view(1).mapped==1000100);CHECK(r.scheduler.view(1).age==10);
    auto backward=f::record(p[13],999999,2,.2f);backward.arrived=1010000;CHECK(r.scheduler.ingest(backward)==l::Code::Order);
    auto future=f::record(p[13],2000000,2,.2f);future.arrived=1999999;CHECK(r.scheduler.ingest(future)==l::Code::Stale);
    l::Clock reset=p[13].clock;reset.epoch=2;reset.sourceAnchor=0;reset.controlAnchor=2000000;
    CHECK(r.scheduler.resetClock(14,reset)==l::Code::Ok);CHECK(r.scheduler.view(1).state==l::State::ClockReset);
    CHECK(!r.scheduler.view(1).present && !r.scheduler.view(1).ageKnown);
    CHECK(r.scheduler.ingest(future)==l::Code::Clock);CHECK(r.scheduler.resetClock(14,reset)==l::Code::Clock);
    auto first=f::record(p[13],0,1,.3f);first.epoch=2;first.arrived=2000000;
    CHECK(r.scheduler.ingest(first)==l::Code::Ok);CHECK(r.scheduler.advance(2000000)==l::Code::Ok);drain(r,2000000);
    CHECK(r.scheduler.view(1).mapped==2000000);CHECK(r.scheduler.advance(2600000)==l::Code::Ok);drain(r,2600000);
    CHECK(r.scheduler.view(1).state==l::State::Stale);
    // Actual irregular timed events and batching remain events, in order. A sequence gap is counted.
    std::vector<uint64_t> beats;
    const auto observe=[](void* ptr,const l::Record& event,l::Time){static_cast<std::vector<uint64_t>*>(ptr)->push_back(event.sequence);};
    uint64_t seq=0;l::Time t=3000000;
    for(l::Time dt:{2000000ull,333334ull,1000000ull,500000ull}) {
        t+=dt;auto beat=f::record(p[2],t,++seq,float(dt)/1000);beat.arrived=9000000;
        CHECK(r.scheduler.ingest(beat)==l::Code::Ok);
    }
    auto gap=f::record(p[2],8500000,6,500);gap.arrived=9000000;CHECK(r.scheduler.ingest(gap)==l::Code::Ok);
    CHECK(r.scheduler.advance(9000000,observe,&beats)==l::Code::Ok);drain(r,9000000);
    CHECK((beats==std::vector<uint64_t>{1,2,3,4,6}));CHECK(r.scheduler.view(0).gaps==1);CHECK(r.scheduler.view(0).last.measured==8500000);
    const auto count=r.scheduler.view(0).accepted;CHECK(r.scheduler.advance(15000000)==l::Code::Ok);drain(r,15000000);
    CHECK(r.scheduler.view(0).accepted==count && r.scheduler.view(0).state==l::State::Stale);
    auto distant=gap;distant.sequence=7;distant.measured=l::kHorizon+1;distant.arrived=16000000;CHECK(r.scheduler.ingest(distant)==l::Code::Clock);
}
static void bounded() {
    auto owned=std::make_unique<Rig>();auto& r=*owned;auto p=f::profiles()[6];CHECK(r.scheduler.declare(p)==l::Code::Ok);
    for(size_t n=0;n<l::kRecords;++n) CHECK(r.scheduler.ingest(f::record(p,n+1,n+1,1))==l::Code::Ok);
    CHECK(r.scheduler.queued()==l::kRecords);CHECK(r.scheduler.counters().highWater==l::kRecords);
    auto dropped=f::record(p,300,257,1);CHECK(r.scheduler.ingest(dropped)==l::Code::Full);CHECK(r.scheduler.counters().overflow==1);
    CHECK(r.scheduler.advance(300)==l::Code::Ok);drain(r,300);
    CHECK(r.scheduler.ingest(f::record(p,301,258,1))==l::Code::Ok);
    CHECK(r.scheduler.advance(301)==l::Code::Ok);drain(r,301);CHECK(r.scheduler.view(0).gaps==1);
    for(size_t n=0;n<l::kSnapshots;++n) {l::Snapshot s;s.at=1000+n;CHECK(r.lane.publish(s)==l::Code::Ok);}
    l::Snapshot full;full.at=2000;CHECK(r.lane.publish(full)==l::Code::Full);
    CHECK(r.scheduler.permission(p.source,p.channel,l::Permission::Revoked,0)==l::Code::Ok);
    const auto receipt=guarded(r,nullptr,nullptr,0,2000);CHECK(receipt.consumed==l::kSnapshots);CHECK(receipt.state[0]==l::State::Missing);
    CHECK(receipt.epoch==r.lane.epoch());CHECK(r.lane.publish(full)==l::Code::Ok);drain(r,2000);
    auto invalid=full;invalid.at=3000;invalid.normalized[0]=NAN;CHECK(r.lane.publish(invalid)==l::Code::Invalid);
    invalid=full;invalid.at=3000;invalid.state[0]=l::State::Fresh;invalid.until[0]=2999;CHECK(r.lane.publish(invalid)==l::Code::Invalid);
}
static void audio() {
    auto owned=std::make_unique<Rig>();auto& r=*owned;const auto doc=document();const auto token=r.start(doc);
    parable::Engine reference(48000);CHECK(reference.load(native(doc))==parable::Status::Ok);
    std::array<float,64> in{},out{};
    const float special[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-0.f,.1f};
    for(size_t block=0;block<32;++block) {
        for(size_t n=0;n<in.size();++n)in[n]=block<2?special[n%7]:.1f*std::sin(float(block*64+n));
        const auto got=guarded(r,in.data(),out.data(),64,block*2000);CHECK(got.patch.token==token);CHECK(!got.timeError);
        for(size_t n=0;n<in.size();++n) {
            const float expected=reference.process(in[n]);CHECK(std::memcmp(&out[n],&expected,sizeof(float))==0);
        }
    }
    auto utility=std::make_unique<Rig>();auto& u=*utility;u.start(document("empty.json"));
    l::Snapshot target;target.at=0;target.until={100000,100000};target.state={l::State::Fresh,l::State::Fresh};target.normalized={0,0};
    CHECK(u.lane.publish(target)==l::Code::Ok);std::array<float,48> x{},y{};x.fill(.25f);
    float gain=1;const float step=2.f/(.02f*48000);
    for(size_t block=0;block<21;++block) {
        const auto got=guarded(u,x.data(),y.data(),48,block*1000);
        for(float sample:y) {gain+= -gain< -step? -step:-gain;CHECK(std::abs(sample-.25f*gain*gain)<1e-6f);}
        CHECK(got.gain[0]>=0 && got.gain[0]<=1);
    }
    CHECK(gain==0);const auto expired=guarded(u,x.data(),y.data(),48,101000);CHECK(expired.state[0]==l::State::Stale);CHECK(expired.gain[0]>0);
    // Control may stop completely; audio TTL alone restores neutral, smoothly.
    for(size_t n=0;n<21;++n)guarded(u,x.data(),y.data(),48,102000+n*1000);
    CHECK(y.back()==.25f);
    const auto backwards=guarded(u,x.data(),y.data(),48,1000);CHECK(backwards.timeError);CHECK(backwards.state[0]==l::State::ClockReset);
    CHECK(guarded(u,nullptr,nullptr,1025,200000).patch.code==a::BlockCode::InvalidBlock);
    // Patch adoption does not reset a global test envelope; no Engine setter is exposed.
    target.at=200000;target.until={300000,300000};target.normalized={.25f,.5f};CHECK(u.lane.publish(target)==l::Code::Ok);
    for(size_t n=0;n<20;++n)guarded(u,x.data(),y.data(),48,200000+n*1000);
    const auto ready=u.runtime.stage(document("manual.json"));CHECK(ready.code==a::Code::Ok);CHECK(u.runtime.publish(ready.token).code==a::Code::Ok);
    const auto adopted=guarded(u,x.data(),y.data(),48,221000);CHECK(adopted.patch.token==ready.token && adopted.patch.adopted);CHECK(adopted.gain[0]==.5f && adopted.gain[1]==.5f);
    CHECK(u.runtime.collect().code==a::Code::Ok);
    u.runtime.requestStop();CHECK(guarded(u,x.data(),y.data(),48,222000).patch.code==a::BlockCode::Stopped);
    for(float value:y)CHECK(value==0);
    u.runtime.stopAfterJoin();u.lane.resetAfterJoin();u.audio.resetAfterJoin();u.start(document("empty.json"));
    CHECK(guarded(u,x.data(),y.data(),48,0).gain[0]==1);CHECK(y.back()==.25f);
}
static void replay() {
    // All heterogeneous cadence fixtures, plus normalized controls, through actual RAT.
    auto rows=f::synthetic(2100000);auto controls=f::read(std::string(LIVING_FIXTURES)+"/trace-a.csv");
    rows.erase(std::remove_if(rows.begin(),rows.end(),[](const auto& r){return r.arrived>=2100000;}),rows.end());
    for(const auto& r:controls)if(r.arrived<2100000)rows.push_back(r);
    std::stable_sort(rows.begin(),rows.end(),[](const auto& x,const auto& y){return x.arrived<y.arrived;});
    std::vector<float> reference;std::vector<uint64_t> controlReference;
    for(size_t chunk:{1u,7u,256u}) {
        auto rig=std::make_unique<Rig>();rig->start(document());for(const auto& p:f::profiles())CHECK(rig->scheduler.declare(p)==l::Code::Ok);
        std::vector<float> signal;std::vector<uint64_t> controlsSeen;size_t cursor=0;
        std::array<float,48> in{},out{};
        for(l::Time t=0;t<2100000;t+=1000) {
            if(t%10000==0) {cursor=f::feed(rig->scheduler,rows,cursor,t,chunk);CHECK(rig->scheduler.advance(t)==l::Code::Ok);}
            for(size_t n=0;n<in.size();++n)in[n]=.1f*std::sin(float(t+n)*.001f);
            const auto got=guarded(*rig,in.data(),out.data(),48,t);
            controlsSeen.push_back(got.target.at);controlsSeen.push_back(got.epoch);
            for(float value:got.gain) {uint32_t bits;std::memcpy(&bits,&value,4);controlsSeen.push_back(bits);}
            signal.insert(signal.end(),out.begin(),out.end());
        }
        cursor=f::feed(rig->scheduler,rows,cursor,2100000,chunk);
        CHECK(rig->scheduler.advance(2100000)==l::Code::Ok);drain(*rig,2100000);
        CHECK(cursor==rows.size());
        if(chunk==1){reference=signal;controlReference=controlsSeen;}else {CHECK(signal==reference);CHECK(controlsSeen==controlReference);}
    }
    // Sparse 300-second measurements remain sparse; no artificial beats or zeros.
    auto sparse=std::make_unique<Rig>();CHECK(sparse->scheduler.declare(f::profiles()[0])==l::Code::Ok);
    CHECK(sparse->scheduler.ingest(sample(0,0,1,70))==l::Code::Ok);
    for(l::Time t=0;t<=320000000;t+=10000000) {CHECK(sparse->scheduler.advance(t)==l::Code::Ok);drain(*sparse,t);}
    CHECK(sparse->scheduler.view(0).accepted==1);CHECK(sparse->scheduler.view(0).last.value[0]==70);CHECK(sparse->scheduler.view(0).state==l::State::Stale);
    for(const char* text:{"bad header\n", "source,measured_us,arrival_us,sequence,value,validity,quality,permission\n14,0,0,1,nan,1,1,1\n",
        "source,measured_us,arrival_us,sequence,value,validity,quality,permission\n14,0,0,1,.5,1,1,1,extra\n"}) {
        bool rejected=false;try{std::istringstream stream(text);f::read(stream);}catch(const std::exception&){rejected=true;}CHECK(rejected);
    }
    bool oversized=false;try{std::istringstream stream(std::string(1024,'x'));f::read(stream);}catch(const std::exception&){oversized=true;}CHECK(oversized);
    bool longSynthetic=false;try{f::synthetic(l::kHorizon);}catch(const std::exception&){longSynthetic=true;}CHECK(longSynthetic);
}
static void throughput() {
    for(size_t budget:{32u*1024,128u*1024}) {
        const auto count=(budget+sizeof(l::Record)-1)/sizeof(l::Record);const size_t seconds=budget==32768?10:1;
        auto r=std::make_unique<Rig>();auto profiles=f::profiles();
        for(size_t i:{5u,6u,7u}) {
            // This budget stimulus has its own synthetic rate, not the nominal
            // device-like cadences used by the listening fixtures.
            profiles[i].acquisitionHz=profiles[i].deliveryHz=float(count)/3;
            CHECK(r->scheduler.declare(profiles[i])==l::Code::Ok);
        }
        size_t cursor=0;const auto start=std::chrono::steady_clock::now();
        for(l::Time tick=0;tick<=seconds*1000000;tick+=10000) {
            while(cursor<count*seconds && cursor*1000000/count<=tick) {
                const size_t i=5+cursor%3;auto rec=f::record(profiles[i],cursor*1000000/count,cursor/3+1,.1f);
                CHECK(r->scheduler.ingest(rec)==l::Code::Ok);++cursor;
            }
            CHECK(r->scheduler.advance(tick)==l::Code::Ok);drain(*r,tick);
        }
        const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        CHECK(r->scheduler.counters().overflow==0);CHECK(cursor==count*seconds);
        std::cout<<"ingest budget_Bps="<<budget<<" actual_record_B="<<sizeof(l::Record)<<" logical_seconds="<<seconds<<" records="<<cursor
                 <<" bytes="<<cursor*sizeof(l::Record)<<" host_wall_us="<<us<<" high_water="<<r->scheduler.counters().highWater<<" drops=0\n";
    }
    // Same 128 KiB/s equivalent concentrated into a single 250 ms batch.
    auto burst=std::make_unique<Rig>();auto p=f::profiles()[6];
    p.acquisitionHz=2048;p.deliveryHz=4;CHECK(burst->scheduler.declare(p)==l::Code::Ok);
    constexpr size_t offered=(128*1024/4)/sizeof(l::Record);
    for(size_t n=0;n<offered;++n) {
        auto rec=f::record(p,n*250000/offered,n+1,1);rec.arrived=250000;
        CHECK(burst->scheduler.ingest(rec)==(n<l::kRecords?l::Code::Ok:l::Code::Full));
    }
    CHECK(burst->scheduler.counters().overflow==offered-l::kRecords);
    CHECK(burst->scheduler.advance(250000)==l::Code::Ok);drain(*burst,250000);
    std::cout<<"unpaced_128KiBps_250ms offered="<<offered<<" accepted="<<l::kRecords<<" dropped="<<offered-l::kRecords<<" policy=drop-new\n";
}
static void stress() {
    const auto doc=document();const auto patch=native(doc);auto rig=std::make_unique<Rig>();
    for(uint32_t epoch=0;epoch<4;++epoch) {
        const auto token=rig->start(doc);std::atomic<uint32_t> done{0},progress{0};std::atomic<bool> finish{false};
        std::thread audio([&]{
            parable::Engine reference(48000);VERIFY(reference.load(patch)==parable::Status::Ok);
            std::array<float,48> in{},out{};in.fill(.1f);std::array<float,2> gain{1,1};
            const float step=2.f/(.02f*48000);uint32_t blocks=0;
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(50);
            while(!finish.load(std::memory_order_acquire)) {
                VERIFY(std::chrono::steady_clock::now()<deadline);const l::Time now=l::Time(blocks)*1000;
                const auto got=guarded(*rig,in.data(),out.data(),48,now);VERIFY(!got.timeError);
                VERIFY(got.patch.token==token || got.patch.code==a::BlockCode::Stopped);
                std::array<float,2> goal{1,1};
                for(size_t d=0;d<2;++d)if(got.state[d]==l::State::Fresh)goal[d]=got.target.normalized[d]*(d==0?2.f:1.f);
                if(got.target.state[0]==l::State::Fresh) {
                    VERIFY(got.target.normalized[1]==1.f-got.target.normalized[0]);
                    VERIFY(got.target.until[0]==got.target.until[1]);
                }
                for(size_t n=0;n<48;++n) {
                    for(size_t d=0;d<2;++d) {const float delta=goal[d]-gain[d];gain[d]+=std::clamp(delta,-step,step);}
                    const float expected=reference.process(in[n]*gain[0])*gain[1];
                    VERIFY(std::isfinite(out[n]) && std::abs(out[n])<=1);
                    if(got.patch.code!=a::BlockCode::Stopped) VERIFY(out[n]==expected);else VERIFY(out[n]==0);
                }
                VERIFY(got.gain==gain);++blocks;progress.store(blocks,std::memory_order_release);
                if(got.patch.code==a::BlockCode::Stopped) {finish.store(true,std::memory_order_release);break;}
            }
            VERIFY(g::clean());done.store(blocks,std::memory_order_release);
        });
        size_t full=0;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(50);
        for(uint32_t n=1;n<=2048;++n) {
            l::Snapshot snapshot;snapshot.at=n*1000;snapshot.until={100000000,100000000};
            snapshot.state={l::State::Fresh,l::State::Fresh};snapshot.normalized[0]=float(n%17)/16;
            snapshot.normalized[1]=1.f-snapshot.normalized[0];
            for(;;) {
                const auto code=rig->lane.publish(snapshot);if(code==l::Code::Ok)break;VERIFY(code==l::Code::Full);++full;
                VERIFY(std::chrono::steady_clock::now()<deadline);std::this_thread::yield();
            }
            if(n%137==0)VERIFY(rig->lane.invalidate()==l::Code::Ok);
            // Producer pacing is outside callback, not a real-time wait.
            while(progress.load(std::memory_order_acquire)+8<n) {VERIFY(std::chrono::steady_clock::now()<deadline);std::this_thread::yield();}
        }
        rig->runtime.requestStop();audio.join();CHECK(done.load()>0 && rig->runtime.stopped());
        rig->runtime.stopAfterJoin();rig->lane.resetAfterJoin();rig->audio.resetAfterJoin();
        std::cout<<"concurrency epoch="<<epoch<<" publications=2048 invalidations=14 blocks="<<done.load()<<" full_retries="<<full<<" reference_exact=1 guards_clean=1\n";
    }
}
int main(int argc,char** argv) {
    if(argc==2 && std::string(argv[1])=="--stress")stress();
    else if(argc==1){contract();timing();bounded();audio();replay();throughput();}
    else return 2;
    std::cout<<"living checks="<<testChecks<<" passed\n";
}
