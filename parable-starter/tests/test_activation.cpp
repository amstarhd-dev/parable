#include "parable_activation.h"
#include "parable_presets.h"
#include "parable_flash_sim.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

using namespace parable;
namespace a=parable::activation;
namespace g=activation_guard;
// No shared CHECK counter in the worker threads (the existing test helper is not atomic).
#define VERIFY(x) do { if(!(x)){std::cerr<<"activation failure "<<__LINE__<<": "<<#x<<'\n';std::abort();} }while(false)
static std::string fixture(const char* name) {
    std::ifstream f(std::string(PATCH_FIXTURES)+"/"+name);CHECK(f.is_open());
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
static patch::ValidatedPatch decode(const std::string& text) {
    auto r=patch::Codec::decode(text);CHECK(std::holds_alternative<patch::ValidatedPatch>(r));return std::get<patch::ValidatedPatch>(r);
}
struct Documents {
    std::array<std::string,4> json;
    std::array<Patch,4> native;
    explicit Documents(bool gemini=false) {
        const auto safe=decode(fixture("empty.json"));
        presets::Geometry geo{163840,4096,256};presets::SimulatedFlash flash(geo);presets::Journal journal(flash,safe);
        const char* names[]={gemini?"gemini-healthy.json":"manual.json",gemini?"gemini-fault.json":"utilities.json","rat-composite.json",gemini?"utilities.json":"rat-reversed.json"};
        for(size_t i=0;i<4;++i) CHECK(journal.save(i,fixture(names[i])).status==presets::Status::Saved);
        flash.powerCycle();presets::Journal reboot(flash,safe);
        for(size_t i=0;i<4;++i) {
            const auto loaded=reboot.load(i);CHECK(loaded.status==presets::Status::Loaded);json[i]=loaded.canonical;
            const auto r=preparation::prepare(decode(json[i]),48000);CHECK(std::holds_alternative<Patch>(r));native[i]=std::get<Patch>(r);
        }
    }
};
static a::Receipt render(a::Runtime& r,const float* in,float* out,size_t n) {
    g::begin();const auto got=r.processBlock(in,out,n);g::end();VERIFY(g::clean());return got;
}
static void invariant(a::Runtime& r) {
    const auto s=r.inspect();VERIFY(s.code==a::Code::Ok);std::array<size_t,5> counts{};
    for(size_t i=0;i<4;++i){const auto& slot=s.slots[i];++counts[size_t(slot.phase)];if(slot.phase!=a::Phase::Free){VERIFY(a::index(slot.token)==i);VERIFY(a::generation(slot.token)>0);}}
    for(size_t i=1;i<5;++i) VERIFY(counts[i]<=1);
    if(s.staging) VERIFY(s.slots[a::index(s.staging)].phase==a::Phase::Staging);
    if(s.pending) VERIFY(s.slots[a::index(s.pending)].phase==a::Phase::Pending);
    if(s.retired) VERIFY(s.slots[a::index(s.retired)].phase==a::Phase::Retired);
}
static a::Token stage(a::Runtime& r,const std::string& text) {
    g::preparation=true;const auto got=r.stage(text);g::preparation=false;VERIFY(got.code==a::Code::Ok);return got.token;
}
static void contract(const Documents& docs) {
    g::selfTest();auto owner=std::make_unique<a::Runtime>(48000);auto& r=*owner;
    CHECK(r.stopped());CHECK(r.collect().code==a::Code::Ok);
    std::array<float,64> in{},out{};in.fill(.1f);
    CHECK(render(r,in.data(),out.data(),in.size()).code==a::BlockCode::Stopped);
    std::array<a::Token,4> tokens{};
    tokens[0]=stage(r,docs.json[0]);CHECK(r.publish(tokens[0]).code==a::Code::Ok);CHECK(r.startAfterJoin().code==a::Code::Ok);
    Engine reference(48000);a::Token referenceToken=0;size_t selected=0;
    auto block=[&](a::BlockCode expected){
        const auto got=render(r,in.data(),out.data(),in.size());CHECK(got.code==expected);CHECK(got.token==tokens[selected]);CHECK(got.frames==in.size());
        if(referenceToken!=got.token){reference=Engine(48000);CHECK(reference.load(docs.native[selected])==Status::Ok);referenceToken=got.token;}
        for(size_t n=0;n<in.size();++n)CHECK(out[n]==reference.process(in[n]));
    };
    block(a::BlockCode::Rendered);CHECK(r.document(tokens[0])==std::optional<patch::ValidatedPatch>(decode(docs.json[0])));
    tokens[1]=stage(r,docs.json[1]);CHECK(r.publish(tokens[1]).code==a::Code::Ok);selected=1;block(a::BlockCode::Rendered);
    tokens[2]=stage(r,docs.json[2]);CHECK(r.publish(tokens[2]).code==a::Code::Ok);
    tokens[3]=stage(r,docs.json[3]);invariant(r);
    const auto full=r.inspect();for(const auto slot:full.slots)CHECK(slot.phase!=a::Phase::Free);
    CHECK(r.publish(tokens[3]).code==a::Code::Busy);CHECK(r.stage(docs.json[0]).code==a::Code::Busy);
    block(a::BlockCode::Deferred); // Still utility Engine, pending RAT untouched.
    CHECK(r.collect().token==tokens[0]);CHECK(!r.document(tokens[0]));CHECK(r.discard(tokens[0]).code==a::Code::InvalidToken);
    selected=2;block(a::BlockCode::Rendered);CHECK(r.publish(tokens[3]).code==a::Code::Ok);block(a::BlockCode::Deferred);
    CHECK(r.collect().token==tokens[1]);selected=3;block(a::BlockCode::Rendered);CHECK(r.collect().token==tokens[2]);
    for(const char* name:{"invalid-unit.json","invalid-revision.json","invalid-disconnected-cycle.json","invalid-nan.json","invalid-inf.json"}) {
        const auto bad=r.stage(fixture(name));CHECK(bad.code==a::Code::InvalidDocument);CHECK(bad.error.has_value());invariant(r);block(a::BlockCode::Rendered);
    }
    for(size_t failure=0;failure<24;++failure) {
        g::failAfter=failure;const auto failed=r.stage(docs.json[0]);g::failAfter=SIZE_MAX;
        CHECK(failed.code==a::Code::AllocationFailed);invariant(r);block(a::BlockCode::Rendered);
    }
    auto mismatch=docs.json[0];const auto pos=mismatch.find("4.80000000e+04");CHECK(pos!=std::string::npos);mismatch.replace(pos,14,"4.41000000e+04");
    const auto badRate=r.stage(mismatch);CHECK(badRate.code==a::Code::PreparationFailed);CHECK(badRate.error.has_value());block(a::BlockCode::Rendered);
    CHECK(r.publish(tokens[0]).code==a::Code::InvalidToken);CHECK(r.discard(tokens[3]).code==a::Code::InvalidToken);
    CHECK(render(r,nullptr,nullptr,1025).code==a::BlockCode::InvalidBlock);CHECK(render(r,nullptr,out.data(),1).code==a::BlockCode::InvalidBlock);
    block(a::BlockCode::Rendered);
    const float values[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),0,.1f};
    for(size_t n=0;n<in.size();++n)in[n]=values[n%7];
    block(a::BlockCode::Rendered);for(float value:out){CHECK(std::isfinite(value));CHECK(std::abs(value)<=1);}
    r.requestStop();CHECK(!r.stopped());CHECK(render(r,in.data(),out.data(),64).code==a::BlockCode::Stopped);CHECK(r.stopped());
    CHECK(r.stage(docs.json[1]).code==a::Code::Ok); // Control-only staging after ACK is safe; cleanup cancels it.
    r.stopAfterJoin();invariant(r);CHECK(r.startAfterJoin().code==a::Code::Ok);CHECK(render(r,in.data(),out.data(),64).code==a::BlockCode::NoActive);
    r.stopAfterJoin(); // Also works when callback ceased without servicing a stop request.
    auto t=stage(r,docs.json[2]);CHECK(r.publish(t).code==a::Code::Ok);r.requestStop();r.stopAfterJoin();CHECK(!r.document(t));
    t=stage(r,docs.json[0]);CHECK(r.discard(t).code==a::Code::Ok);CHECK(r.discard(t).code==a::Code::InvalidToken);
    uint32_t next=a::kMaxGeneration;const auto last=a::detail::issue(next,3);CHECK(a::generation(last)==a::kMaxGeneration);CHECK(a::detail::issue(next,0)==0);
    a::Runtime invalid(NAN);CHECK(invalid.stage(docs.json[0]).code==a::Code::PreparationFailed);
    for(float rate:{8000.f,44100.f,96000.f,192000.f}) {
        auto adjusted=docs.json[0];const auto where=adjusted.find("4.80000000e+04");adjusted.replace(where,14,std::to_string(rate));
        a::Runtime other(rate);const auto ready=stage(other,adjusted);CHECK(other.publish(ready).code==a::Code::Ok);CHECK(other.startAfterJoin().code==a::Code::Ok);
        const auto native=preparation::prepare(decode(adjusted),rate);CHECK(std::holds_alternative<Patch>(native));Engine expected(rate);CHECK(expected.load(std::get<Patch>(native))==Status::Ok);
        CHECK(render(other,in.data(),out.data(),64).token==ready);for(size_t i=0;i<64;++i)CHECK(out[i]==expected.process(in[i]));other.stopAfterJoin();
        auto rat=docs.json[2];rat.replace(rat.find("4.80000000e+04"),14,std::to_string(rate));const auto result=other.stage(rat);
        CHECK(result.code==(rate>=32000 && rate<=96000?a::Code::Ok:a::Code::InvalidDocument));other.stopAfterJoin();
    }
    std::cout<<"layout engine="<<sizeof(Engine)<<" typed="<<sizeof(patch::ValidatedPatch)<<" bundle="<<a::Runtime::bundleBytes()<<" pool="<<4*a::Runtime::bundleBytes()<<" runtime="<<sizeof(a::Runtime)<<" shared_atomics=16\n";
    std::cout<<"control_cpp_allocations="<<g::control.cppAlloc<<" cumulative_requested_bytes="<<g::control.bytes<<" callback_allocations=0 callback_locks=0 callback_io=0\n";
}
struct Event {uint64_t sequence;char kind;a::Token token;size_t frames;std::array<a::Slot,4> slots;};
static void stopAndTiming(const Documents& docs) {
    auto owner=std::make_unique<a::Runtime>(48000);auto& r=*owner;
    const auto first=stage(r,docs.json[0]);CHECK(r.publish(first).code==a::Code::Ok);CHECK(r.startAfterJoin().code==a::Code::Ok);render(r,nullptr,nullptr,0);
    const auto second=stage(r,docs.json[1]);CHECK(r.publish(second).code==a::Code::Ok);render(r,nullptr,nullptr,0);
    const auto pending=stage(r,docs.json[2]);CHECK(r.publish(pending).code==a::Code::Ok);const auto staging=stage(r,docs.json[3]);
    r.requestStop();CHECK(render(r,nullptr,nullptr,0).code==a::BlockCode::StopDeferred);CHECK(!r.stopped());
    CHECK(r.stage(docs.json[0]).code==a::Code::Stopping);CHECK(r.publish(staging).code==a::Code::Stopping);
    CHECK(r.collect().token==first);CHECK(render(r,nullptr,nullptr,0).code==a::BlockCode::Stopped);CHECK(r.stopped());
    CHECK(r.startAfterJoin().code==a::Code::NotStopped);r.stopAfterJoin();CHECK(!r.document(pending));CHECK(!r.document(staging));
    CHECK(r.startAfterJoin().code==a::Code::Ok);
    using Clock=std::chrono::steady_clock;
    for(size_t i=0;i<4;++i){const auto begin=Clock::now();const auto token=stage(r,docs.json[i]);const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-begin).count();CHECK(r.discard(token).code==a::Code::Ok);std::cout<<"prepare_slot="<<i<<" ns="<<ns<<'\n';}
    std::array<int64_t,256> samples{};
    for(auto& ns:samples){CHECK(r.collect().code==a::Code::Ok);const auto token=stage(r,docs.json[0]);CHECK(r.publish(token).code==a::Code::Ok);const auto begin=Clock::now();const auto receipt=render(r,nullptr,nullptr,0);ns=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-begin).count();CHECK(receipt.adopted);CHECK(receipt.token==token);}
    std::sort(samples.begin(),samples.end());std::cout<<"boundary_only_ns min="<<samples.front()<<" median="<<samples[128]<<" max="<<samples.back()<<" samples=256 (includes host timer/guard overhead; not a deadline proof)\n";r.stopAfterJoin();
}
static uint32_t random(uint32_t& n){n^=n<<13;n^=n>>17;n^=n<<5;return n;}
static void stress(const Documents& docs,const char* path) {
    auto runtime=std::make_unique<a::Runtime>(48000);auto& r=*runtime;
    std::array<uint8_t,16384> mapping{};
    std::atomic<uint64_t> order{0};std::atomic<unsigned> credits{0};std::atomic<bool> quit{false};
    std::vector<Event> control,audio;control.reserve(50000);audio.reserve(30000);
    uint32_t seed=0x50415211;size_t busy=0,published=0,adopted=0,totalFrames=0,acks=0;
    auto log=[&](char kind,a::Token token){const auto s=r.inspect();VERIFY(s.code==a::Code::Ok);VERIFY(control.size()<control.capacity());control.push_back({order.fetch_add(1),kind,token,0,s.slots});};
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(140);
    auto grant=[&]{if(credits.load()<3)credits.fetch_add(1);};
    auto advance=[&]{VERIFY(std::chrono::steady_clock::now()<deadline);grant();std::this_thread::sleep_for(std::chrono::microseconds(20));};
    for(size_t epoch=0;epoch<4;++epoch) {
        quit.store(false);credits.store(0);VERIFY(r.startAfterJoin().code==a::Code::Ok);
        std::thread worker([&]{
            std::optional<Engine> reference;a::Token last=0;uint32_t noise=uint32_t(1234+epoch);size_t frames=0;
            while(!quit.load(std::memory_order_acquire)) {
                if(!credits.load()){std::this_thread::yield();continue;}
                credits.fetch_sub(1);const size_t n=1+random(noise)%64;std::array<float,64> in{},out{};
                for(size_t i=0;i<n;++i)in[i]=float(int(random(noise)%2001)-1000)/5000.f;
                if(frames%19==0)in[0]=NAN;
                if(frames%23==0)in[n-1]=std::numeric_limits<float>::max();
                const auto got=render(r,in.data(),out.data(),n);VERIFY(got.frames==n);VERIFY(got.code!=a::BlockCode::ProtocolFault);
                if(got.token && got.token!=last) {
                    VERIFY(got.adopted);VERIFY(a::generation(got.token)>a::generation(last));
                    reference.emplace(48000);VERIFY(reference->load(docs.native[mapping[a::generation(got.token)]])==Status::Ok);last=got.token;++adopted;
                }
                for(size_t i=0;i<n;++i){const float expected=got.token?reference->process(in[i]):0;VERIFY(out[i]==expected);VERIFY(std::isfinite(out[i]));VERIFY(std::abs(out[i])<=1);}
                VERIFY(audio.size()<audio.capacity());audio.push_back({order.fetch_add(1),'A',got.token,n,{}});frames+=n;
            }
            totalFrames+=frames;
        });
        for(size_t i=0;i<1024;++i) {
            const auto freed=r.collect();VERIFY(freed.code==a::Code::Ok);if(freed.token){++acks;VERIFY(!r.document(freed.token));log('F',freed.token);}
            if(i%17==0){const auto invalid=r.stage("{}");VERIFY(invalid.code==a::Code::InvalidDocument);log('X',0);}
            const auto which=random(seed)%4;const auto ready=stage(r,docs.json[which]);VERIFY(a::generation(ready)<mapping.size());mapping[a::generation(ready)]=uint8_t(which);log('S',ready);
            for(;;){const auto pub=r.publish(ready);if(pub.code==a::Code::Ok)break;VERIFY(pub.code==a::Code::Busy);++busy;invariant(r);const auto ack=r.collect();VERIFY(ack.code==a::Code::Ok);if(ack.token){++acks;log('F',ack.token);}advance();}
            ++published;log('P',ready);invariant(r);for(size_t n=0,nMax=1+random(seed)%3;n<nMax;++n)grant();
            if(i%13==0)std::this_thread::yield();
        }
        // Last pending publication may be canceled; no wait in the Runtime itself.
        r.requestStop();log('Q',0);
        while(!r.stopped()){const auto ack=r.collect();VERIFY(ack.code==a::Code::Ok);if(ack.token){++acks;log('F',ack.token);}advance();}
        quit.store(true,std::memory_order_release);worker.join();r.stopAfterJoin();log('J',0);invariant(r);
        for(const auto slot:r.inspect().slots)VERIFY(slot.phase==a::Phase::Free);
    }
    VERIFY(published==4096);VERIFY(adopted>=4000);VERIFY(busy>0);VERIFY(totalFrames>adopted);
    std::vector<a::Token> identities;for(const auto& e:audio)if(e.token)identities.push_back(e.token);
    std::sort(identities.begin(),identities.end());identities.erase(std::unique(identities.begin(),identities.end()),identities.end());VERIFY(identities.size()==adopted);
    size_t loggedFrames=0;for(const auto& e:audio)loggedFrames+=e.frames;VERIFY(loggedFrames==totalFrames);
    std::vector<a::Token> freed;for(const auto& e:control)if(e.kind=='F'){VERIFY(std::binary_search(identities.begin(),identities.end(),e.token));freed.push_back(e.token);}
    std::sort(freed.begin(),freed.end());VERIFY(std::adjacent_find(freed.begin(),freed.end())==freed.end());VERIFY(freed.size()==acks);
    if(path){std::vector<Event> events=control;events.insert(events.end(),audio.begin(),audio.end());std::sort(events.begin(),events.end(),[](const Event&x,const Event&y){return x.sequence<y.sequence;});std::ofstream f(path);VERIFY(f.is_open());f<<"observation,event,token,generation,index,frames,slot0,slot1,slot2,slot3\n";for(const auto& e:events){f<<e.sequence<<','<<e.kind<<','<<e.token<<','<<a::generation(e.token)<<','<<a::index(e.token)<<','<<e.frames;for(const auto s:e.slots)f<<','<<int(s.phase)<<':'<<s.token;f<<'\n';}}
    std::cout<<"stress publications="<<published<<" adopted="<<adopted<<" busy="<<busy<<" retirement_acks="<<acks<<" frames="<<totalFrames<<" joined_restarts=4 callback_allocations=0 callback_locks=0 callback_io=0\n";
    std::cout<<"test_log_capacity_bytes="<<(control.capacity()+audio.capacity())*sizeof(Event)<<" mapping_bytes="<<sizeof(mapping)<<" worker_io_buffers=512 reference_engine="<<sizeof(Engine)<<'\n';
}
int main(int argc,char**argv){const bool gemini=argc>1&&std::string(argv[1])=="--gemini-stress";const Documents docs(gemini);if(argc>1 && (std::string(argv[1])=="--stress"||gemini))stress(docs,argc>2?argv[2]:nullptr);else {contract(docs);stopAndTiming(docs);}}
