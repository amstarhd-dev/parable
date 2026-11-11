// Host-only stopped-save / control-prepare / audio-block example. No hardware IO.
#include "parable_activation.h"
#include "parable_presets.h"
#include "parable_flash_sim.h"
#include <charconv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

int main(int argc,char**argv) {
    using namespace parable;
    auto read=[](const char* name){std::ifstream f(std::string(PATCH_FIXTURES)+"/"+name);if(!f)throw std::runtime_error("fixture unavailable");return std::string(std::istreambuf_iterator<char>(f),{});};
    try {
        const auto decoded=patch::Codec::decode(read("empty.json"));
        if(!std::holds_alternative<patch::ValidatedPatch>(decoded)) return 1;
        presets::SimulatedFlash flash({163840,4096,256});presets::Journal journal(flash,std::get<patch::ValidatedPatch>(decoded));
        const char* names[]={"manual.json","utilities.json","rat-composite.json","rat-reversed.json"};
        for(size_t i=0;i<4;++i)if(journal.save(i,read(names[i])).status!=presets::Status::Saved)return 1;
        // No saves after playback starts. Real flash/audio coexistence is undecided.
        auto runtime=std::make_unique<activation::Runtime>(48000);
        if(runtime->startAfterJoin().code!=activation::Code::Ok)return 1;
        const int count=argc>1?argc-1:4;size_t frame=0;
        for(int step=0;step<count;++step) {
            unsigned slot=unsigned(step);
            if(argc>1){const auto len=std::strlen(argv[step+1]);const auto parsed=std::from_chars(argv[step+1],argv[step+1]+len,slot);if(parsed.ec!=std::errc{} || parsed.ptr!=argv[step+1]+len || slot>3){std::cerr<<"preset numbers must be 0..3\n";return 2;}}
            const auto document=journal.load(slot);if(!document.usable())return 1;
            if(runtime->collect().code!=activation::Code::Ok)return 1;
            const auto ready=runtime->stage(document.canonical);if(ready.code!=activation::Code::Ok || runtime->publish(ready.token).code!=activation::Code::Ok)return 1;
            uint64_t hash=14695981039346656037ull;size_t rendered=0;
            for(size_t block=0;block<16;++block){
                std::array<float,48> in{},out{};for(auto& x:in)x=float(int(frame++%97)-48)/500.f;
                const auto receipt=runtime->processBlock(in.data(),out.data(),in.size());
                if(receipt.token!=ready.token || receipt.frames!=in.size())return 1;
                for(float x:out){uint32_t bits;std::memcpy(&bits,&x,sizeof(bits));hash^=bits;hash*=1099511628211ull;}rendered+=receipt.frames;
            }
            std::cout<<"slot="<<slot<<" token="<<ready.token<<" generation="<<activation::generation(ready.token)<<" frames="<<rendered<<" hash="<<std::hex<<hash<<std::dec<<'\n';
        }
        // This single-thread harness is quiescent here. A threaded caller joins first.
        runtime->stopAfterJoin();
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
