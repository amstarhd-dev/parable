// Host-only requested-payload accounting. Headers/allocator bookkeeping excluded.
// Scalar FFTConvolver uses new/new[]; no SSE aligned malloc or foreign FFT backend.
#include "wallace2_graph.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
namespace {
bool tracking=false;size_t live=0,peak=0,calls=0;
struct alignas(std::max_align_t) Header {size_t size;bool counted;};
}
void* operator new(size_t n){
    auto* h=static_cast<Header*>(std::malloc(sizeof(Header)+(n?n:1)));if(!h)throw std::bad_alloc();
    h->size=n;h->counted=tracking;if(tracking){live+=n;peak=std::max(peak,live);++calls;}return h+1;
}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {if(p){auto* h=static_cast<Header*>(p)-1;if(h->counted)live-=h->size;std::free(h);}}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
int main(int argc,char**argv){
    if(argc!=2)return 1;
    std::cout<<"{\"unit\":\"requested new/new[] payload bytes; excludes allocator metadata and call stack\",\"cases\":[";bool first=true;
    for(const std::string name:{"jensen","v30"}){
        std::ifstream f(std::string(argv[1])+"/"+name+"-48k.f32",std::ios::binary|std::ios::ate);if(!f)return 2;const auto bytes=f.tellg();std::vector<float> h(size_t(bytes)/4);f.seekg(0);
        // Measurement executable supports the observed little-endian host only.
        uint32_t endian=1;if(*reinterpret_cast<unsigned char*>(&endian)!=1)return 3;
        f.read(reinterpret_cast<char*>(h.data()),bytes);if(!f)return 4;
        for(bool full:{false,true}){peak=calls=0;if(live)return 5;tracking=true;
            std::unique_ptr<wallace2::Cabinet> cab;std::unique_ptr<wallace2::Graph> graph;bool ok;
            if(full){graph=std::make_unique<wallace2::Graph>();ok=graph->prepare(h.data(),h.size());}
            else {cab=std::make_unique<wallace2::Cabinet>();ok=cab->prepare(48000,h.data(),h.size());}
            tracking=false;if(!ok)return 6;const auto retained=live,high=peak,count=calls;cab.reset();graph.reset();if(live)return 7;
            if(!first)std::cout<<',';first=false;std::cout<<"{\"ir\":\""<<name<<"\",\"full_graph\":"<<(full?"true":"false")<<",\"retained_bytes\":"<<retained<<",\"peak_prepare_bytes\":"<<high<<",\"allocations\":"<<count<<'}';
        }
    }
    std::cout<<"],\"caller_block_bytes\":"<<sizeof(wallace2::Block)<<",\"unfreed_tracked_bytes\":"<<live<<"}\n";
}
