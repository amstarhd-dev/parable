#include "parable_activation.h"
#include "parable_presets.h"
#include "parable_flash_sim.h"
#include "cmos_measure.h"
#include "activation_guard.h"
#include "test_check.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>

using namespace parable;
namespace p=parable::patch;
namespace a=parable::activation;
namespace g=activation_guard;
using Json=nlohmann::json;
static std::string fixture(const char* name) {
    std::ifstream f(std::string(PATCH_FIXTURES)+"/"+name);CHECK(bool(f));return {std::istreambuf_iterator<char>(f),{}};
}
static p::ValidatedPatch decode(const std::string& text) {
    auto d=p::Codec::decode(text);if(const auto* e=std::get_if<p::Error>(&d))std::cerr<<e->field<<": "<<e->rule<<'\n';
    CHECK(std::holds_alternative<p::ValidatedPatch>(d));return std::get<p::ValidatedPatch>(d);
}
static Patch prepare(const p::ValidatedPatch& d) {
    auto n=preparation::prepare(d,d.sampleRateHz());CHECK(std::holds_alternative<Patch>(n));return std::get<Patch>(n);
}
static void contract() {
    for(const char* name:{"gemini-healthy","gemini-fault"}) {
        const auto d=decode(fixture((std::string(name)+".json").c_str()));
        CHECK(p::Codec::encode(d)==fixture((std::string(name)+".canonical.json").c_str()));
        CHECK(decode(p::Codec::encode(d))==d);
        const auto n=prepare(d);CHECK(n.count==1 && n.stages[0].type==Type::GeminiCmos);
        CHECK(n.stages[0].gemini.biasLeak==(std::string(name)=="gemini-fault"));
    }
    const auto base=Json::parse(fixture("gemini-healthy.json"));
    const auto reject=[](const Json& d,p::ErrorCode code){const auto result=p::Codec::decode(d.dump());CHECK(std::holds_alternative<p::Error>(result));CHECK(std::get<p::Error>(result).code==code);};
    for(const char* control:{"drive","volume"}) {
        for(double value:{-.01,1.01}) {auto d=base;d["processors"][0]["parameters"][control]["value"]=value;reject(d,p::ErrorCode::OutOfRange);}
        auto d=base;d["processors"][0]["parameters"].erase(control);reject(d,p::ErrorCode::MissingField);
        d=base;d["processors"][0]["parameters"][control]["unit"]="Hz";reject(d,p::ErrorCode::InvalidUnit);
    }
    for(const Json& value:{Json(0),Json(1),Json(.5),Json("true"),Json(nullptr)}) {auto d=base;d["processors"][0]["parameters"]["biasLeak"]["value"]=value;reject(d,p::ErrorCode::WrongType);}
    auto d=base;d["processors"][0]["parameters"]["biasLeak"]["unit"]="normalized";reject(d,p::ErrorCode::InvalidUnit);
    d=base;d["processors"][0]["parameters"].erase("biasLeak");reject(d,p::ErrorCode::MissingField);
    d=base;d["processors"][0]["parameters"]["Rb"]={{"unit","ohm"},{"value",220000}};reject(d,p::ErrorCode::UnknownField);
    d=base;d["processors"][0]["revision"]=2;reject(d,p::ErrorCode::UnsupportedVersion);
    for(float rate:{32000.f,44100.f,48000.f,96000.f}) {d=base;d["sampleRate"]["value"]=rate;CHECK(prepare(decode(d.dump())).count==1);}
    for(int rate:{8000,31999,96001,192000}) {d=base;d["sampleRate"]["value"]=rate;reject(d,p::ErrorCode::OutOfRange);}
    for(bool mixed:{false,true}) {
        d=base;auto second=mixed?Json::parse(fixture("rat.json"))["processors"][0]:d["processors"][0];
        second["id"]=8;second["enabled"]=false;d["processors"].push_back(second);
        d["routing"]=Json::array({{{"from","input"},{"to",7}},{{"from",7},{"to",8}},{{"from",8},{"to","output"}}});
        reject(d,p::ErrorCode::LimitExceeded);
    }
    auto native=prepare(decode(base.dump()));native.count=2;native.stages[1].setRat({.5f,.5f,.5f});
    CHECK(validate(native,48000)==Status::TooManyNonlinear);
    native.stages[1]=native.stages[0];CHECK(validate(native,48000)==Status::TooManyNonlinear);
    native.count=1;CHECK(validate(native,192000)==Status::UnsupportedCmosRate);
}
static void references() {
    dsp::Cmos model;std::ifstream dc(std::string(GEMINI_FIXTURES)+"/bias-dc.csv");CHECK(bool(dc));std::string line;std::getline(dc,line);double dcPeak=0;
    while(std::getline(dc,line)) {
        const auto f=cmoslab::fields(line);CHECK(f.size()==7);auto c=dsp::geminiCircuit(f[0]=="bias_leak");c.railVolts=std::stod(f[1]);
        if(f[0]=="feedback_low")c.secondFeedbackOhms=100e3;
        CHECK(model.configure(96000,{.75f,.5f},c,dsp::CmosRate::Reference1x));
        const auto n=model.lastNodes();const double actual[]={n.gate1,n.output1,n.gate2,n.output2,n.jackVolts};
        for(size_t j=0;j<5;++j)dcPeak=std::max(dcPeak,std::abs(actual[j]-std::stod(f[j+2])));
        const double gateKcl=(n.gate2-n.output2)/c.secondFeedbackOhms+c.secondBiasSiemens*n.gate2;
        const double drainKcl=dsp::cmos_detail::inverter(n.gate2,n.output2,c.railVolts).value+(n.output2-n.gate2)/c.secondFeedbackOhms;
        CHECK(std::abs(gateKcl)<1e-12);CHECK(std::abs(drainKcl)<1e-10);
        for(size_t i=0;i<4096;++i)CHECK(std::abs(model.process(0))<1e-8);
        CHECK(model.failedSolves()==0);
    }
    CHECK(dcPeak<1e-5);
    double referencePeak=0;
    for(bool production:{false,true}) {
    std::ifstream transient(std::string(GEMINI_FIXTURES)+(production?"/bias-production-4x.csv":"/bias-transient.csv"));CHECK(bool(transient));std::getline(transient,line);
    std::string current;size_t sample=0,count=0;double sum=0,peak=0;
    while(std::getline(transient,line)) {
        const auto f=cmoslab::fields(line);CHECK(f.size()==8);const auto key=f[0]+f[1];
        if(key!=current) {current=key;sample=0;CHECK(model.configure(96000,{.75f,.5f},dsp::geminiCircuit(f[0]=="bias_leak"),production?dsp::CmosRate::Oversampled4x:dsp::CmosRate::Reference1x));}
        const auto target=std::stoul(f[2]);const double amplitude=std::stod(f[1]);
        for(;sample<=target;++sample)model.process(static_cast<float>(amplitude*std::sin(2*cmoslab::pi*997*sample/96000)));
        const auto n=model.lastNodes();const double actual[]={n.gate1,n.output1,n.gate2,n.output2,n.jackVolts};
        for(size_t j=0;j<5;++j) {const double error=std::abs(actual[j]-std::stod(f[j+3]));sum+=error*error;peak=std::max(peak,error);++count;}
        CHECK(model.failedSolves()==0);CHECK(n.residualAmps<=1e-9);
    }
    const double rms=std::sqrt(sum/count);std::cout<<"Gemini independent SPICE: mode="<<(production?4:1)<<" DC peak V="<<dcPeak<<" transient RMS/peak V="<<rms<<'/'<<peak<<" nodes="<<count<<'\n';
    CHECK(count==3840);CHECK(rms<.005);
    // 1x is the intentionally lower-bandwidth comparison, not the shipped mode:
    // permit <=1% of the 9 V rail, disclose its old 50 mV target miss. Actual 4x
    // must meet 50 mV AND improve peak error by at least 2x, without fitted shift.
    if(production) {CHECK(peak<.05);CHECK(peak<referencePeak*.5);}
    else {CHECK(peak<.09);referencePeak=peak;}
    }
    // Additional independent points at the exact real-DI / target playback rates.
    std::ifstream playback(std::string(GEMINI_FIXTURES)+"/bias-playback-4x.csv");CHECK(bool(playback));std::getline(playback,line);
    std::string current;size_t sample=0,count=0;double sum=0,peak=0;
    while(std::getline(playback,line)) {
        const auto f=cmoslab::fields(line);CHECK(f.size()==9);const auto key=f[0]+f[1]+f[2];const float rate=std::stof(f[2]);
        if(key!=current){current=key;sample=0;CHECK(model.configure(rate,{.75f,.5f},dsp::geminiCircuit(f[0]=="bias_leak")));}
        const auto target=std::stoul(f[3]);const double amplitude=std::stod(f[1]);
        for(;sample<=target;++sample)model.process(static_cast<float>(amplitude*std::sin(2*cmoslab::pi*997*sample/rate)));
        const auto n=model.lastNodes();const double actual[]={n.gate1,n.output1,n.gate2,n.output2,n.jackVolts};
        for(size_t j=0;j<5;++j){const double error=std::abs(actual[j]-std::stod(f[j+4]));sum+=error*error;peak=std::max(peak,error);++count;}
        CHECK(model.failedSolves()==0);
    }
    std::cout<<"Gemini SPICE exact playback 44.1/48 kHz 4x RMS/peak V="<<std::sqrt(sum/count)<<'/'<<peak<<" nodes="<<count<<'\n';
    CHECK(count==3840);CHECK(std::sqrt(sum/count)<.005);CHECK(peak<.05);
}
static void circuit() {
    dsp::Cmos a,b;CHECK(a.configure(48000,{.75f,.5f}));CHECK(b.configure(48000,{.75f,.5f},dsp::geminiCircuit(true)));
    CHECK(std::abs(a.biasVolts()-3.482808651)<1e-8);CHECK(std::abs(b.secondGateBiasVolts()-1.622727171)<1e-8);
    CHECK(b.secondOutputBiasVolts()>8.99);CHECK(a.biasVolts()==b.biasVolts());
    double peak=0;for(size_t i=0;i<2048;++i) {const float x=i<256?.15f*std::sin(.13f*i):0;const float y=b.process(x);CHECK(std::isfinite(y)&&std::abs(y)<=1);if(i>=256)peak=std::max(peak,std::abs(double(y)));}
    CHECK(peak>1e-6);CHECK(b.failedSolves()==0); // stateful passive recovery, no fallback glitch.
    auto invalid=dsp::geminiCircuit(true);invalid.secondBiasSiemens=-1;CHECK(!b.configure(48000,{},invalid));
    invalid.secondBiasSiemens=1.01e-5;CHECK(!b.configure(48000,{},invalid));
    for(float rate:{32000.f,44100.f,96000.f})for(bool fault:{false,true}) {
        CHECK(b.configure(rate,{1,.5f},dsp::geminiCircuit(fault)));
        for(size_t n=0;n<8192;++n){float y=b.process(.15f*std::sin(float(n)*.031f));CHECK(std::isfinite(y)&&std::abs(y)<=1);}
        CHECK(b.failedSolves()==0);CHECK(b.maximumLoopEvaluations()<=16);
    }
    // Absolute quiet-input suppression; do not normalize near-silence to fake a voice.
    const auto h=cmoslab::spectrum({}, {.75f,.5f},85,.005);
    const auto f=cmoslab::spectrum(dsp::geminiCircuit(true),{.75f,.5f},85,.005);
    CHECK(f.fundamental<h.fundamental/1000);
}
static void integration() {
    g::selfTest();
    for(bool fault:{false,true}) {
        const auto d=decode(fixture(fault?"gemini-fault.json":"gemini-healthy.json"));const auto n=prepare(d);
        Engine engine(48000);CHECK(engine.load(n)==Status::Ok);dsp::Cmos reference;CHECK(reference.configure(48000,{.75f,.5f},dsp::geminiCircuit(fault)));
        const float edges[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),0,.1f};
        for(size_t i=0;i<16000;++i) {
            const float x=i<35?edges[i%7]:.15f*std::sin(float(i)*.071f);
            g::begin();const float y=engine.process(x);g::end();CHECK(g::clean());
            const float expected=reference.process(x)*n.outputTrim;CHECK(y==expected);CHECK(std::isfinite(y)&&std::abs(y)<=1);
            if(i==200) {auto bad=n;bad.stages[0].gemini.drive=NAN;CHECK(engine.load(bad)==Status::InvalidParameter);}
        }
        auto disabled=n;disabled.stages[0].enabled=false;CHECK(engine.load(disabled)==Status::Ok);
        const auto old=engine.cmosNodes();for(size_t i=0;i<1000;++i)CHECK(engine.process(.1f)==.1f*n.outputTrim);
        CHECK(engine.cmosNodes().output2==old.output2);
    }
    // Four saved presets are documents, not four permanently allocated CMOS states.
    const auto safe=decode(fixture("empty.json"));presets::SimulatedFlash flash({163840,4096,256});presets::Journal journal(flash,safe);
    const char* names[]={"gemini-healthy.json","gemini-fault.json","rat-composite.json","utilities.json"};
    auto runtime=std::make_unique<a::Runtime>(48000);CHECK(runtime->startAfterJoin().code==a::Code::Ok);
    for(size_t slot=0;slot<4;++slot)CHECK(journal.save(slot,fixture(names[slot])).status==presets::Status::Saved);
    flash.powerCycle();
    for(size_t step=0;step<12;++step) {
        const auto saved=journal.load(step%4);CHECK(saved.usable());const auto typed=decode(saved.canonical);const auto n=prepare(typed);
        Engine reference(48000);CHECK(reference.load(n)==Status::Ok);CHECK(runtime->collect().code==a::Code::Ok);
        const auto ready=runtime->stage(saved.canonical);CHECK(ready.code==a::Code::Ok);CHECK(runtime->publish(ready.token).code==a::Code::Ok);
        std::array<float,48> in{},out{};in.fill(.1f);
        g::begin();const auto receipt=runtime->processBlock(in.data(),out.data(),48);g::end();CHECK(g::clean());
        CHECK(receipt.token==ready.token && receipt.adopted);for(size_t i=0;i<48;++i)CHECK(out[i]==reference.process(in[i]));
    }
    runtime->stopAfterJoin();
    struct Legacy {float rate,volume;Patch patch;std::array<float,kMaxStages> state,coeff;std::optional<dsp::Rat> rat;};
    CHECK(sizeof(Engine)==sizeof(Legacy));CHECK(sizeof(Stage)==24);CHECK(sizeof(Patch)==204);
    std::cout<<"layout: CMOS="<<sizeof(dsp::Cmos)<<" Engine="<<sizeof(Engine)<<" typed="<<sizeof(p::ValidatedPatch)<<" bundle="<<a::Runtime::bundleBytes()<<" Runtime="<<sizeof(a::Runtime)<<" (shared slot, no growth)\n";
}
int main(){contract();references();circuit();integration();std::cout<<"Gemini checks="<<testChecks<<" passed\n";}
