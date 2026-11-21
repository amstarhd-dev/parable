// Linux/GNU host laboratory interception. NEVER included in ARM firmware.
#include "host_tracker.h"
#include <cstdlib>
#include <new>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
namespace resource_bench::tracking {
bool enabled=false,audio=false;Metrics metrics;
struct Entry {void* pointer=nullptr;size_t bytes=0;};static Entry entries[8192];
void start() noexcept {metrics={};for(auto& e:entries)e={};enabled=true;}
static void remove(void* p) noexcept {if(!p)return;for(auto& e:entries)if(e.pointer==p){metrics.live-=e.bytes;e={};return;}}
static void add(void* p,size_t n) noexcept {if(!enabled||!p)return;for(auto& e:entries)if(!e.pointer){e={p,n};metrics.live+=n;if(metrics.live>metrics.peak)metrics.peak=metrics.live;++metrics.allocations;return;}++metrics.overflow;}
}
extern "C" {void* __real_malloc(size_t);void* __real_calloc(size_t,size_t);void* __real_realloc(void*,size_t);void __real_free(void*);
void* __wrap_malloc(size_t n){using namespace resource_bench::tracking;if(audio)++metrics.callbackAllocations;void* p=__real_malloc(n);add(p,n);return p;}
void* __wrap_calloc(size_t n,size_t z){using namespace resource_bench::tracking;if(audio)++metrics.callbackAllocations;void* p=__real_calloc(n,z);add(p,n*z);return p;}
void* __wrap_realloc(void* p,size_t n){using namespace resource_bench::tracking;if(audio)++metrics.callbackAllocations;void* q=__real_realloc(p,n);if(q||!n){remove(p);add(q,n);}return q;}
void __wrap_free(void* p){using namespace resource_bench::tracking;if(audio&&p)++metrics.callbackFrees;remove(p);__real_free(p);}
int __real_pthread_mutex_lock(pthread_mutex_t*);int __wrap_pthread_mutex_lock(pthread_mutex_t* p){using namespace resource_bench::tracking;if(audio)++metrics.locks;return __real_pthread_mutex_lock(p);}
int __real_pthread_mutex_trylock(pthread_mutex_t*);int __wrap_pthread_mutex_trylock(pthread_mutex_t* p){using namespace resource_bench::tracking;if(audio)++metrics.locks;return __real_pthread_mutex_trylock(p);}
ssize_t __real_read(int,void*,size_t);ssize_t __wrap_read(int fd,void* p,size_t n){using namespace resource_bench::tracking;if(audio)++metrics.io;return __real_read(fd,p,n);}
ssize_t __real_write(int,const void*,size_t);ssize_t __wrap_write(int fd,const void* p,size_t n){using namespace resource_bench::tracking;if(audio)++metrics.io;return __real_write(fd,p,n);}
FILE* __real_fopen(const char*,const char*);FILE* __wrap_fopen(const char* p,const char* m){using namespace resource_bench::tracking;if(audio)++metrics.io;return __real_fopen(p,m);}
size_t __real_fwrite(const void*,size_t,size_t,FILE*);size_t __wrap_fwrite(const void* p,size_t n,size_t z,FILE* f){using namespace resource_bench::tracking;if(audio)++metrics.io;return __real_fwrite(p,n,z,f);}
}
void* operator new(size_t n){if(void* p=__wrap_malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {__wrap_free(p);}void operator delete[](void* p) noexcept {__wrap_free(p);}
void operator delete(void* p,size_t) noexcept {__wrap_free(p);}void operator delete[](void* p,size_t) noexcept {__wrap_free(p);}
