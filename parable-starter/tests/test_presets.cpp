#include "parable_presets.h"
#include "parable_flash_sim.h"
#include "preset_crc.h"
#include "test_check.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

using namespace parable;
using namespace parable::presets;
static std::string file(const std::string& path) {
    std::ifstream f(path, std::ios::binary); CHECK(f.is_open());
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static std::string fixture(const char* name) { return file(std::string(PATCH_FIXTURES)+"/"+name); }
static patch::ValidatedPatch decoded(std::string_view text) {
    auto value = patch::Codec::decode(text);
    CHECK(std::holds_alternative<patch::ValidatedPatch>(value));
    return std::get<patch::ValidatedPatch>(value);
}
static std::string canonical(std::string_view text) { return patch::Codec::encode(decoded(text)); }
static std::string replaced(std::string text, const std::string& old, const std::string& value) {
    const auto pos = text.find(old); CHECK(pos != std::string::npos); text.replace(pos, old.size(), value); return text;
}
static const auto safe = decoded(fixture("empty.json"));
static Geometry geometry(size_t program = 8, size_t erase = 64) {
    Geometry g{SIZE_MAX, erase, program}; g.capacity = layout(g).totalBytes; return g;
}
// Independent bitwise CRC oracle, unlike the borrowed nibble evaluator.
static uint32_t oracleCrc(const void* data, size_t size) {
    uint32_t crc = UINT32_MAX; const auto* p = static_cast<const uint8_t*>(data);
    for (size_t i=0; i<size; ++i) { crc ^= p[i]; for (int j=0;j<8;++j) crc = (crc>>1) ^ (0xedb88320u & (0u-(crc&1u))); }
    return ~crc;
}
static void put(std::vector<uint8_t>& h, size_t off, uint64_t n, size_t size) {
    for (size_t i=0;i<size;++i) { h[off+i] = uint8_t(n); n >>= 8; }
}
// Independent fixture builder can place deliberately invalid/future records.
static void plant(SimulatedFlash& f, size_t slot, size_t bank, const std::string& payload,
                  uint64_t gen = 1, uint16_t major = 1, uint32_t kind = 1) {
    const auto l = layout(f.geometry()); std::vector<uint8_t> image(l.bankBytes, 0xff);
    std::fill_n(image.begin(), 64, 0);
    const std::string magic("PARPSET\0",8); std::copy(magic.begin(),magic.end(),image.begin());
    put(image,8,major,2); put(image,10,64,2); put(image,12,slot,4); put(image,16,gen,8);
    put(image,24,payload.size(),4); put(image,28,oracleCrc(payload.data(),payload.size()),4); put(image,32,kind,4);
    put(image,60,oracleCrc(image.data(),60),4);
    std::copy(payload.begin(),payload.end(),image.begin()+l.headerBytes);
    std::fill(image.end()-f.geometry().programBytes,image.end(),0xa5);
    for (size_t i=0;i<image.size();++i) CHECK(f.corrupt(l.bank(slot,bank)+i, f.bytes()[l.bank(slot,bank)+i]^image[i]));
}
static void headerField(SimulatedFlash& f, size_t slot, size_t bank, size_t off, uint64_t n, size_t count) {
    const auto start = layout(f.geometry()).bank(slot,bank);
    std::vector<uint8_t> h(f.bytes().begin()+start, f.bytes().begin()+start+64);
    put(h,off,n,count); put(h,60,oracleCrc(h.data(),60),4);
    for (size_t i=0;i<64;++i) CHECK(f.corrupt(start+i,f.bytes()[start+i]^h[i]));
}
static std::string hex(const uint8_t* p, size_t n) {
    std::ostringstream out; for(size_t i=0;i<n;++i) out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(p[i]); return out.str();
}
static void contract() {
    CHECK(detail::crc32("123456789",9)==0xcbf43926); CHECK(detail::crc32("",0)==0);
    const auto g=geometry(256,4096); CHECK(g.capacity==163840); CHECK(layout(g).bankBytes==20480);
    SimulatedFlash flash(g); Journal store(flash,safe);
    for(size_t slot=0;slot<4;++slot) { const auto r=store.load(slot); CHECK(r.status==Status::DefaultEmpty); CHECK(r.canonical==patch::Codec::encode(safe)); CHECK(r.generation==0); }
    const std::array<std::string,4> docs={fixture("manual.json"),fixture("utilities.json"),fixture("rat-composite.json"),fixture("rat-reversed.json")};
    for(size_t slot=0;slot<4;++slot) CHECK(store.save(slot,docs[slot]).status==Status::Saved);
    for(size_t slot=0;slot<4;++slot) {
        flash.powerCycle(); Journal reboot(flash,safe); const auto r=reboot.load(slot);
        CHECK(r.status==Status::Loaded); CHECK(r.generation==1); CHECK(r.canonical==canonical(docs[slot]));
        CHECK(decoded(r.canonical)==decoded(docs[slot])); // Stable IDs and serial order included.
    }
    const auto before=flash.bytes(); CHECK(store.save(0,docs[2]).generation==2);
    CHECK(std::equal(before.begin()+2*layout(g).bankBytes,before.end(),flash.bytes().begin()+2*layout(g).bankBytes));
    CHECK(store.clear(0).generation==3); CHECK(store.load(0).status==Status::DefaultCleared);
    CHECK(std::equal(before.begin()+2*layout(g).bankBytes,before.end(),flash.bytes().begin()+2*layout(g).bankBytes));
    for(size_t slot=1;slot<4;++slot) CHECK(store.load(slot).canonical==canonical(docs[slot]));
    // Match independently packed Python/zlib bytes, not native struct layout.
    SimulatedFlash golden(g); Journal writer(golden,safe);
    for(int i=0;i<9;++i) CHECK(writer.save(2,docs[2]).status==Status::Saved);
    const auto latest=writer.load(2); CHECK(latest.generation==9);
    const auto start=layout(g).bank(2,latest.activeBank);
    CHECK(hex(golden.bytes().data()+start,64)+"\n"==file(std::string(PRESET_FIXTURES)+"/slot2-gen9.header.hex"));
    const auto clean=flash.bytes();
    const std::array<std::string,5> invalid={"{",fixture("invalid-unit.json"),fixture("invalid-disconnected-cycle.json"),replaced(docs[2],"\"revision\": 1","\"revision\": 2"),"{}"};
    for(const auto& bad:invalid) CHECK(store.save(1,bad).status==Status::InvalidDocument);
    CHECK(store.save(4,docs[0]).status==Status::InvalidSlot); CHECK(store.clear(4).status==Status::InvalidSlot); CHECK(!store.load(4).usable());
    CHECK(flash.bytes()==clean);
}
static void corruption() {
    const auto a=canonical(fixture("manual.json")), b=canonical(fixture("rat-composite.json"));
    SimulatedFlash base(geometry()); Journal store(base,safe); CHECK(store.save(0,a).status==Status::Saved); CHECK(store.save(0,b).generation==2);
    const auto l=store.storageLayout();
    auto damaged=base; CHECK(damaged.corrupt(l.bank(0,1)+l.headerBytes+5,1)); Journal recover(damaged,safe);
    CHECK(recover.load(0).status==Status::Recovered); CHECK(recover.load(0).canonical==a); CHECK(recover.load(0).badBanks==2);
    CHECK(damaged.corrupt(l.headerBytes+5,1)); CHECK(recover.load(0).status==Status::DefaultCorrupt);
    for(const auto& field:std::array<std::pair<size_t,uint64_t>,4>{{{24,16385},{24,0},{12,3},{16,0}}}) {
        auto f=base; headerField(f,0,1,field.first,field.second,field.first==16?8:4); Journal j(f,safe); CHECK(j.load(0).status==Status::Recovered); CHECK(j.load(0).canonical==a);
    }
    auto badHeader=base; CHECK(badHeader.corrupt(l.bank(0,1)+60,1)); Journal h(badHeader,safe); CHECK(h.load(0).status==Status::Recovered);
    for (const auto& payload:std::array<std::string,3>{"{}", b+" ", replaced(b,"\"normalized\"","\"volts\"")}) {
        auto f=base; plant(f,0,1,payload,2); Journal j(f,safe); CHECK(j.load(0).status==Status::Recovered); CHECK(j.load(0).canonical==a);
    }
    for(const auto& payload:std::array<std::string,6>{replaced(b,"\"version\":1","\"version\":2"),replaced(b,"\"revision\":1","\"revision\":2"),replaced(b,"\"Rat\"","\"FutureRat\""),
        replaced(b,"\"version\":1","\"version\":2,\"futureField\":true"),
        replaced(b,"\"revision\":1","\"revision\":2,\"futureField\":true"),
        replaced(replaced(b,"\"revision\":1","\"revision\":2,\"futureField\":true"),"\"version\":1","\"version\":2")}) {
        auto f=base; plant(f,0,1,payload,2); Journal j(f,safe); const auto bytes=f.bytes();
        CHECK(j.load(0).status==Status::Unsupported); CHECK(!j.load(0).usable()); CHECK(j.save(0,a).status==Status::Unsupported); CHECK(j.clear(0).status==Status::Unsupported); CHECK(f.bytes()==bytes);
    }
    for(const auto& future:std::array<std::pair<uint16_t,uint32_t>,2>{{{2,1},{1,99}}}) {
        auto f=base; plant(f,0,1,b,1,future.first,future.second); Journal j(f,safe); CHECK(j.load(0).status==Status::Unsupported); CHECK(j.save(0,a).status==Status::Unsupported);
    }
    auto f=base; plant(f,0,1,b,1); Journal equal(f,safe); CHECK(equal.load(0).status==Status::Conflict); CHECK(equal.save(0,b).status==Status::Conflict);
    plant(f,0,1,a,1); CHECK(equal.load(0).status==Status::Loaded); CHECK(equal.load(0).activeBank==0);
    plant(f,0,1,b,UINT64_MAX); const auto bytes=f.bytes(); CHECK(equal.load(0).generation==UINT64_MAX);
    CHECK(equal.save(0,a).status==Status::GenerationExhausted); CHECK(equal.clear(0).status==Status::GenerationExhausted); CHECK(f.bytes()==bytes);
}
static size_t cuts=0, calls=0;
static uint64_t digest=14695981039346656037ull;
static void hash(uint64_t n) { for(int i=0;i<8;++i){digest^=uint8_t(n);digest*=1099511628211ull;n>>=8;} }
static void powerCuts(const SimulatedFlash& initial, const std::string& old, const std::string& next, bool clear, bool exhaustive) {
    auto success=initial; success.powerCycle(); success.trace(true); Journal store(success,safe);
    const auto result=clear?store.clear(0):store.save(0,next); CHECK(result.status==Status::Saved);
    const size_t total=success.mutations(); std::vector<size_t> points;
    if(exhaustive) { points.reserve(total+1); for(size_t n=0;n<=total;++n) points.push_back(n); }
    else {
        points={0,1,total/2,total-1,total}; size_t n=0;
        for(const auto& e:success.events()) if(e.operation!=SimulatedFlash::Operation::Read) { points.push_back(n); if(e.bytes>1) points.push_back(n+e.bytes/2); n+=e.bytes; points.push_back(n-1); points.push_back(n); }
        std::sort(points.begin(),points.end()); points.erase(std::unique(points.begin(),points.end()),points.end());
    }
    size_t oldCount=0, newCount=0;
    for(const auto n:points) {
        auto flash=initial; flash.powerCycle(); flash.trace(false); flash.cutAfter(n); Journal j(flash,safe);
        const auto saved=clear?j.clear(0):j.save(0,next); CHECK(saved.status!=Status::Saved);
        const auto bytes=flash.bytes(); flash.powerCycle(); CHECK(flash.bytes()==bytes); Journal reboot(flash,safe); const auto got=reboot.load(0);
        CHECK(got.usable()); CHECK(got.canonical==old || got.canonical==next);
        CHECK(got.generation==result.generation-1 || got.generation==result.generation);
        if(n==total) CHECK(saved.status==Status::CommitUncertain);
        if(got.generation==result.generation) { CHECK(got.canonical==next); CHECK(n==total); ++newCount; }
        else { CHECK(got.canonical==old); ++oldCount; }
        CHECK(std::equal(initial.bytes().begin()+2*store.storageLayout().bankBytes,initial.bytes().end(),flash.bytes().begin()+2*store.storageLayout().bankBytes));
        hash(n);hash(got.generation);hash(static_cast<unsigned>(got.status));++cuts;
    }
    // Every operation boundary, including each readback and lost commit acknowledgment.
    for(size_t call=0;call<success.calls();++call) {
        auto flash=initial; flash.powerCycle(); flash.trace(false); flash.failOnCall(call); Journal j(flash,safe);
        const auto r=clear?j.clear(0):j.save(0,next); CHECK(r.status!=Status::Saved);
        flash.powerCycle(); Journal reboot(flash,safe); const auto got=reboot.load(0); CHECK(got.usable()); CHECK(got.canonical==old || got.canonical==next);
        CHECK(got.generation==result.generation-1 || got.generation==result.generation);
        if(call==success.calls()-1) { CHECK(r.status==Status::CommitUncertain); CHECK(got.canonical==next); }
        hash(call); hash(got.generation); ++calls;
    }
    std::cout<<"matrix kind="<<(clear?"clear":"save")<<" program="<<initial.geometry().programBytes
        <<" erase="<<initial.geometry().eraseBytes<<" prior_generation="<<result.generation-1
        <<" byte_cuts="<<points.size()<<" old="<<oldCount<<" new="<<newCount<<" io_failures="<<success.calls()<<"\n";
}
static std::string largeDocument() {
    // Maximum count/high IDs; one RAT plus seven longest utility shapes.
    std::string p="{\"format\":\"parable.patch\",\"version\":1,\"sampleRate\":{\"unit\":\"Hz\",\"value\":48000},\"outputTrim\":{\"unit\":\"linear\",\"value\":0.5},\"processors\":[";
    for(size_t i=0;i<8;++i) {
        if(i) p+=",";
        p+="{\"id\":"+std::to_string(UINT32_MAX-i)+",\"revision\":1,\"enabled\":false,";
        p+=i ? "\"type\":\"OnePoleLowpass\",\"parameters\":{\"cutoff\":{\"unit\":\"Hz\",\"value\":2000}}}"
            : "\"type\":\"Rat\",\"parameters\":{\"distortion\":{\"unit\":\"normalized\",\"value\":0.5},\"filter\":{\"unit\":\"normalized\",\"value\":0.75},\"volume\":{\"unit\":\"normalized\",\"value\":0.9}}}";
    }
    p+="],\"routing\":[{\"from\":\"input\",\"to\":"+std::to_string(UINT32_MAX)+"}";
    for(size_t i=0;i<7;++i) p+=",{\"from\":"+std::to_string(UINT32_MAX-i)+",\"to\":"+std::to_string(UINT32_MAX-i-1)+"}";
    return p+",{\"from\":"+std::to_string(UINT32_MAX-7)+",\"to\":\"output\"}]}";
}
static void faults() {
    const auto a=canonical(fixture("empty.json")), b=canonical(fixture("manual.json"));
    SimulatedFlash empty(geometry()); powerCuts(empty,a,b,false,true);
    SimulatedFlash initial(geometry()); Journal j(initial,safe); CHECK(j.save(0,b).status==Status::Saved); CHECK(j.save(0,a).status==Status::Saved);
    powerCuts(initial,a,b,false,true); powerCuts(initial,a,a,true,true);
    const auto large=largeDocument(); CHECK(decoded(large).count()==8); const auto full=canonical(large);
    SimulatedFlash big(geometry(256,4096)); Journal k(big,safe); CHECK(k.save(0,b).status==Status::Saved); CHECK(k.save(0,a).status==Status::Saved);
    std::string padded=large; padded.append(patch::kMaxDocumentBytes-padded.size(),' '); CHECK(padded.size()==16384); CHECK(k.save(1,padded).status==Status::Saved); CHECK(k.load(1).canonical==full);
    const auto before=big.bytes(); CHECK(k.save(1,padded+" ").status==Status::DocumentTooLarge); CHECK(big.bytes()==before);
    powerCuts(big,a,full,false,false);
    for(const auto pg:std::array<size_t,3>{1,16,64}) {
        SimulatedFlash other(geometry(pg,256)); Journal setup(other,safe); CHECK(setup.save(0,a).status==Status::Saved);
        powerCuts(other,a,b,false,false);
    }
    // Fixed-seed repeated resets with ongoing generations, no PRNG/time dependency.
    uint32_t seed=0x50415210; std::string expected=a; uint64_t generation=2;
    for(size_t i=0;i<512;++i) {
        seed^=seed<<13; seed^=seed>>17; seed^=seed<<5;
        big.powerCycle(); const auto next=(i&1)?b:full;
        const size_t budget=seed%(layout(big.geometry()).bankBytes+4096); big.cutAfter(budget); Journal attempt(big,safe); const auto r=attempt.save(0,next);
        big.powerCycle(); Journal reboot(big,safe); const auto got=reboot.load(0); CHECK(got.usable());
        CHECK((got.generation==generation && got.canonical==expected) || (got.generation==generation+1 && got.canonical==next));
        if(r.status==Status::Saved) CHECK(got.generation==generation+1);
        expected=got.canonical; generation=got.generation;hash(generation);hash(budget);
    }
    std::cout<<"large_canonical_bytes="<<full.size()<<" max_input_bytes="<<padded.size()<<" random_resets=512\n";
}
static void backendAndFailures() {
    SimulatedFlash f(geometry(8,64)); std::array<uint8_t,8> zeros{}, ones{}; ones.fill(0xff);
    CHECK(f.program(0,zeros.data(),8)==Io::Ok); CHECK(f.program(0,ones.data(),8)==Io::ProgramViolation);
    CHECK(f.program(1,zeros.data(),8)==Io::Alignment); CHECK(f.erase(1,64)==Io::Alignment); CHECK(f.read(SIZE_MAX,ones.data(),8)==Io::Bounds);
    CHECK(f.erase(0,64)==Io::Ok); CHECK(f.program(0,ones.data(),8)==Io::Ok);
    for(const auto g:std::array<Geometry,5>{{{100,0,8},{100,64,0},{100,63,8},{100,64,512},{100,64,8}}}) {
        SimulatedFlash bad(g); Journal j(bad,safe); const auto before=bad.bytes(); CHECK(!j.load(0).usable()); CHECK(j.save(0,fixture("empty.json")).status!=Status::Saved); CHECK(bad.bytes()==before); CHECK(bad.calls()==0);
    }
    auto g=geometry(); SimulatedFlash small(g); Journal misaligned(small,safe,1); CHECK(misaligned.load(0).status==Status::InvalidGeometry);
    SimulatedFlash tooSmall({g.capacity-1,g.eraseBytes,g.programBytes}); Journal full(tooSmall,safe);
    CHECK(full.load(0).status==Status::StorageFull); CHECK(full.save(0,fixture("empty.json")).status==Status::StorageFull); CHECK(tooSmall.calls()==0);
    SimulatedFlash worn(g,1); Journal j(worn,safe); const auto a=fixture("manual.json"),b=fixture("rat-composite.json");
    CHECK(j.save(0,a).status==Status::Saved); CHECK(j.save(0,b).status==Status::Saved); const auto prior=worn.bytes();
    const auto r=j.save(0,a); CHECK(r.status==Status::StorageError); CHECK(r.io==Io::WornOut); CHECK(worn.bytes()==prior); CHECK(j.load(0).canonical==canonical(b));
    const auto wear=worn.erases(); worn.powerCycle(); CHECK(worn.erases()==wear); CHECK(j.save(0,a).io==Io::WornOut);
    // A nonzero partition base must not change sentinels outside the owned journal.
    g.capacity+=128; SimulatedFlash partition(g); CHECK(partition.corrupt(0,0x55)); CHECK(partition.corrupt(g.capacity-1,0x55));
    Journal p(partition,safe,64); CHECK(p.save(3,a).status==Status::Saved); CHECK(partition.bytes()[0]==0xaa); CHECK(partition.bytes().back()==0xaa);
    // Deterministic partial erase/program and wear retain bytes across cycling.
    SimulatedFlash prefix(geometry()); prefix.cutAfter(3); CHECK(prefix.program(0,zeros.data(),8)==Io::PowerLost);
    CHECK(prefix.bytes()[0]==0 && prefix.bytes()[2]==0 && prefix.bytes()[3]==0xff); CHECK(prefix.read(0,ones.data(),8)==Io::PowerLost);
    prefix.powerCycle(); CHECK(prefix.erase(0,64)==Io::Ok); CHECK(prefix.erases()[0]==1);
    // A backend can return Ok yet store damaged bits. Readback must catch each phase.
    class Damaging final : public Storage {
    public:
        SimulatedFlash& flash; size_t target; bool onErase;
        Damaging(SimulatedFlash& f,size_t t,bool e):flash(f),target(t),onErase(e){}
        Geometry geometry() const noexcept override { return flash.geometry(); }
        Io read(size_t o,void* p,size_t n) override { return flash.read(o,p,n); }
        Io program(size_t o,const void* p,size_t n) override {
            const auto r=flash.program(o,p,n); if(r==Io::Ok && !onErase && o==target) CHECK(flash.corrupt(o,1)); return r;
        }
        Io erase(size_t o,size_t n) override {
            const auto r=flash.erase(o,n); if(r==Io::Ok && onErase && o==target) CHECK(flash.corrupt(o,1)); return r;
        }
    };
    SimulatedFlash original(geometry()); Journal first(original,safe); CHECK(first.save(0,a).status==Status::Saved);
    const auto l=first.storageLayout(); const size_t start=l.bank(0,1);
    for(int phase=0;phase<3;++phase) {
        auto damaged=original;
        Damaging adapter(damaged,phase==0?start:phase==1?start+l.headerBytes:start+l.bankBytes-damaged.geometry().programBytes,phase==0);
        Journal writer(adapter,safe); const auto result=writer.save(0,b);
        CHECK(result.status==(phase==2?Status::CommitUncertain:Status::VerificationFailed));
        damaged.powerCycle(); Journal reboot(damaged,safe); CHECK(reboot.load(0).canonical==canonical(a)); CHECK(reboot.load(0).generation==1);
    }
}
int main() {
    contract(); corruption(); backendAndFailures(); faults();
    std::cout<<"byte_cuts="<<cuts<<" io_failures="<<calls<<" deterministic_digest="<<std::hex<<digest<<"\n";
}
