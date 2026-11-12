// Actual saved canonical patch -> PAR-011 Runtime -> test-only envelope wrapper.
#include "living_fixture.h"
#include "parable_presets.h"
#include "parable_flash_sim.h"
#include "soundforge/excitation.h"
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>

namespace l=parable::living;
namespace a=parable::activation;
namespace f=living_fixture;
using f::require;
static std::string patchFile(const char* name) {
    std::ifstream file(std::string(PATCH_FIXTURES)+"/"+name);require(bool(file),"Missing patch");
    return {std::istreambuf_iterator<char>(file),{}};
}
struct Observations {
    std::ofstream stream;
    explicit Observations(const std::filesystem::path& p):stream(p) {
        stream<<std::setprecision(9)<<"source,channel,kind,unit,origin,delivery,timebase,epoch,sequence,measured_us,arrival_us,mapped_us,delay_us,validity,quality,confidence_known,confidence,permission,x,y,z\n";
    }
    static void observed(void* opaque,const l::Record& r,l::Time mapped) noexcept {
        auto& s=static_cast<Observations*>(opaque)->stream;
        s<<r.source<<','<<r.channel<<','<<int(r.kind)<<','<<int(r.unit)<<','<<int(r.origin)<<','<<int(r.delivery)<<','
         <<r.timebase<<','<<r.epoch<<','<<r.sequence<<','<<r.measured<<','<<r.arrived<<','<<mapped<<','<<r.arrived-mapped<<','
         <<int(r.validity)<<','<<int(r.quality)<<','<<bool(r.flags&l::ConfidenceKnown)<<','<<r.confidence<<','<<int(r.permission)<<','
         <<r.value[0]<<','<<r.value[1]<<','<<r.value[2]<<'\n';
    }
};
static lab::Wave render(const std::string& document,const lab::Wave& input,
                        const std::vector<l::Record>& rows,const std::filesystem::path& out,
                        const std::string& name,size_t chunk,bool baseline) {
    auto runtime=std::make_unique<a::Runtime>(float(input.rate));
    const auto ready=runtime->stage(document);require(ready.code==a::Code::Ok,"Stage failed");
    require(runtime->publish(ready.token).code==a::Code::Ok,"Publish failed");
    require(runtime->startAfterJoin().code==a::Code::Ok,"Start failed");
    auto lane=std::make_unique<l::Lane>();auto scheduler=std::make_unique<l::Scheduler>(*lane);
    for(const auto& p:f::profiles()) require(scheduler->declare(p)==l::Code::Ok,"Profile rejected");
    auto audio=std::make_unique<l::Audio>(*lane,*runtime,input.rate);
    // Independent unchanged native Engine reference establishes unity sample parity.
    const auto decoded=parable::patch::Codec::decode(document);
    const auto prepared=parable::preparation::prepare(std::get<parable::patch::ValidatedPatch>(decoded),float(input.rate));
    parable::Engine reference(float(input.rate));require(reference.load(std::get<parable::Patch>(prepared))==parable::Status::Ok,"Reference failed");
    lab::Wave result{input.rate,std::vector<float>(input.samples.size())};
    Observations observed(out/(name+"-observations.csv"));
    std::ofstream trace(out/(name+"-controls.csv")),actions(out/(name+"-permissions.csv"));
    trace<<std::setprecision(9)<<"control_us,patch_token,epoch,pre_state,post_state,pre_present,post_present,pre_age_known,post_age_known,pre_quality,post_quality,pre_measure_us,post_measure_us,pre_arrival_us,post_arrival_us,pre_age_us,post_age_us,pre_target_normalized,post_target_normalized,pre_gain,post_gain\n";
    actions<<"control_us,source,permission\n";
    std::array<bool,16> granted{};granted.fill(true);size_t cursor=0;
    for(size_t frame=0;frame<input.samples.size();frame+=48) {
        const auto now=static_cast<l::Time>(frame/48)*1000;
        if(now%10000==0) {
            // Explicit synthetic fixture actions, not authorization from a sensor value.
            while(!baseline && cursor<rows.size() && rows[cursor].arrived<=now) {
                for(size_t n=0;n<chunk && cursor<rows.size() && rows[cursor].arrived<=now;++n,++cursor) {
                    const auto& r=rows[cursor];const auto id=r.source;
                    const bool grant=r.permission==l::Permission::Granted;
                    if(granted[id]!=grant) {
                        require(scheduler->permission(id,1,r.permission,700000000)==l::Code::Ok,"Fixture consent action failed");
                        granted[id]=grant;actions<<now<<','<<id<<','<<int(r.permission)<<'\n';
                    }
                    if(grant) require(scheduler->ingest(r)==l::Code::Ok,"Demo ingestion failed");
                }
            }
            require(scheduler->advance(now,Observations::observed,&observed)==l::Code::Ok,"Advance failed");
        }
        const auto receipt=audio->processBlock(input.samples.data()+frame,result.samples.data()+frame,48,now);
        require(receipt.patch.token==ready.token && receipt.patch.frames==48 && !receipt.timeError,"Patch/time changed unexpectedly");
        if(baseline) for(size_t i=frame;i<frame+48;++i) require(result.samples[i]==reference.process(input.samples[i]),"Unity changed existing Engine output");
        if(now%10000==0) {
            const auto pre=scheduler->view(13),post=scheduler->view(14);
            trace<<now<<','<<receipt.patch.token<<','<<receipt.epoch<<','<<int(receipt.state[0])<<','<<int(receipt.state[1])<<','
                 <<pre.present<<','<<post.present<<','<<pre.ageKnown<<','<<post.ageKnown<<','<<int(pre.last.quality)<<','<<int(post.last.quality)<<','
                 <<pre.last.measured<<','<<post.last.measured<<','<<pre.last.arrived<<','<<post.last.arrived<<','
                 <<pre.age<<','<<post.age<<','<<receipt.target.normalized[0]<<','<<receipt.target.normalized[1]<<','
                 <<receipt.gain[0]<<','<<receipt.gain[1]<<'\n';
        }
    }
    runtime->requestStop();require(audio->processBlock(nullptr,nullptr,0,input.samples.size()*1000000ull/input.rate).patch.code==a::BlockCode::Stopped,"Stop failed");
    runtime->stopAfterJoin();lane->resetAfterJoin();audio->resetAfterJoin();
    trace.flush();actions.flush();observed.stream.flush();
    require(bool(trace)&&bool(actions)&&bool(observed.stream),"Cannot finish traces");
    lab::write(out/(name+".wav"),result);return result;
}
int main(int argc,char** argv) {
    try {
        std::filesystem::path out;size_t chunk=7;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];require(i+1<argc,"Missing argument");
            if(arg=="--output")out=argv[++i];else if(arg=="--chunk")chunk=static_cast<size_t>(f::integer(argv[++i]));else require(false,"Unknown argument");
        }
        require(!out.empty() && chunk>0 && chunk<=l::kRecords,"Use --output DIR [--chunk 1..256]");
        std::filesystem::create_directories(out);
        const auto safe=parable::patch::Codec::decode(patchFile("empty.json"));
        parable::presets::SimulatedFlash flash({163840,4096,256});
        parable::presets::Journal journal(flash,std::get<parable::patch::ValidatedPatch>(safe));
        require(journal.save(0,patchFile("rat-composite.json")).status==parable::presets::Status::Saved,"Save failed");
        flash.powerCycle();const auto saved=journal.load(0);require(saved.usable(),"Saved patch unavailable");
        std::ofstream patch(out/"saved.patch.json");patch<<saved.canonical;patch.flush();require(bool(patch),"Patch write failed");
        const auto input=soundforge::syntheticPlucks(); // existing owned 9 s / 48 kHz excitation.
        lab::write(out/"00_input.wav",input);
        const auto baseline=render(saved.canonical,input,{},out,"01_unmodulated",chunk,true);
        auto raw=f::synthetic(9000000);
        std::array<lab::Wave,2> waves;
        for(size_t i=0;i<2;++i) {
            auto rows=f::read(std::string(LIVING_FIXTURES)+(i==0?"/trace-a.csv":"/trace-b.csv"));rows.insert(rows.end(),raw.begin(),raw.end());
            std::stable_sort(rows.begin(),rows.end(),[](const auto& x,const auto& y){return x.arrived<y.arrived;});
            waves[i]=render(saved.canonical,input,rows,out,i==0?"02_pre_test":"03_post_test",chunk,false);
        }
        std::ofstream report(out/"report.json");report<<std::setprecision(12)<<"{\n  \"synthetic_only\": true,\n  \"rate\": 48000,\n  \"frames\": "<<input.samples.size()<<",\n  \"unity_max_error\": 0,\n  \"renders\": [\n";
        for(size_t i=0;i<2;++i) {
            double sum=0;for(size_t n=0;n<input.samples.size();++n) {double d=double(waves[i].samples[n])-baseline.samples[n];sum+=d*d;}
            const double difference=std::sqrt(sum/input.samples.size());require(difference>.001,"Modulation not distinct");
            const auto stats=soundforge::stats(waves[i].samples);require(stats.peak<1,"Unbounded render");
            report<<"    {\"fixture\": \""<<(i==0?"a":"b")<<"\", \"rms_difference_from_unity\": "<<difference<<", \"peak\": "<<stats.peak<<", \"rms\": "<<stats.rms<<"}"<<(i==0?",":"")<<'\n';
        }
        double sum=0;for(size_t n=0;n<input.samples.size();++n) {double d=double(waves[0].samples[n])-waves[1].samples[n];sum+=d*d;}
        const auto diff=std::sqrt(sum/input.samples.size());require(diff>.001,"Fixtures not distinct");
        report<<"  ],\n  \"between_fixtures_rms_difference\": "<<diff<<"\n}\n";report.flush();require(bool(report),"Report failed");
        std::ofstream readme(out/"LISTEN.txt");readme<<"PAR-012 host plumbing experiment. All observations, controls and plucks are synthetic, not wearable capture or recorded guitar.\n"
            <<"00 input; 01 unchanged saved canonical RAT composite at unity; 02 changing pre gain; 03 changing post gain.\n"
            <<"Same saved.patch.json, Engine and 9-second 48 kHz excitation. Raw levels; no RMS matching. PCM16 quantization only.\n"
            <<"Test-only pre=2*normalized, post=normalized. Linear bounded slew; hold until measurement TTL, then slew to unity.\n"
            <<"Control ticks=10 ms, blocks=1 ms. Fixture A dropout 2.6-3.5 s, missing 4-4.5 s, revoked 5-6 s. B bad quality 3-3.5 s, dropout 4.1-5 s, revoked 6-7 s.\n"
            <<"Unbound raw synthetic HR/IBI/EDA/motion/ECG/etc do not cause musical changes. No physiological mapping.\n"
            <<"CSV enum definitions and source profiles: include/parable_living.h, tools/living_fixture.h and docs/LIVING_SOURCE_REQUIREMENTS.md.\n"
            <<"No hardware timing, real sensor acquisition, physiological interpretation, musical approval or pilot dependency is proven.\n";
        readme.flush();require(bool(readme),"Readme failed");std::cout<<"Synthetic saved-patch WAVs/traces: "<<out<<"; fixture RMS difference="<<diff<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
