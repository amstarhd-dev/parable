// Linux host interposition for executable/static-object calls; not a universal OS tracer.
#include "activation_guard.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <pthread.h>
#include <unistd.h>
namespace activation_guard {
thread_local bool callback=false,preparation=false;
thread_local Counts audio{},control{};
thread_local size_t failAfter=SIZE_MAX;
static Counts* watching() noexcept { return callback?&audio:preparation?&control:nullptr; }
void selfTest() {
    pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
    callback=true;
    void* volatile p=std::malloc(7); std::free(p);
    // std::stable_sort's temporary buffer uses this overload. Keep allocation
    // and deletion in the same interposition family under ASan as well.
    void* volatile q=::operator new(7,std::nothrow); ::operator delete(q);
    pthread_mutex_lock(&mutex); pthread_mutex_unlock(&mutex);
    const auto ignored=::write(-1,"",0); (void)ignored;
    callback=false;
    if(!audio.cppAlloc || !audio.cppFree || !audio.cAlloc || !audio.cFree || !audio.locks || !audio.io) std::abort();
    audio={}; pthread_mutex_destroy(&mutex);
}
}
void* operator new(size_t n) {
    if(activation_guard::failAfter!=SIZE_MAX && !activation_guard::failAfter--) throw std::bad_alloc();
    if(auto* c=activation_guard::watching()){++c->cppAlloc;c->bytes+=n;}
    if(void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t n) {return ::operator new(n);}
void* operator new(size_t n,std::align_val_t a) {
    if(auto* c=activation_guard::watching()){++c->cppAlloc;c->bytes+=n;}
    void* p=nullptr;if(posix_memalign(&p,size_t(a),n?n:1)==0)return p;throw std::bad_alloc();
}
void* operator new[](size_t n,std::align_val_t a){return ::operator new(n,a);}
void* operator new(size_t n,const std::nothrow_t&) noexcept {
    try{return ::operator new(n);}catch(...){return nullptr;}
}
void* operator new[](size_t n,const std::nothrow_t& tag) noexcept {return ::operator new(n,tag);}
void* operator new(size_t n,std::align_val_t a,const std::nothrow_t&) noexcept {
    try{return ::operator new(n,a);}catch(...){return nullptr;}
}
void* operator new[](size_t n,std::align_val_t a,const std::nothrow_t& tag) noexcept {return ::operator new(n,a,tag);}
static void drop(void* p) noexcept {
    if(auto* c=activation_guard::watching())++c->cppFree;
    std::free(p);
}
void operator delete(void* p) noexcept{drop(p);} void operator delete[](void* p) noexcept{drop(p);}
void operator delete(void* p,size_t) noexcept{drop(p);} void operator delete[](void* p,size_t) noexcept{drop(p);}
void operator delete(void* p,std::align_val_t) noexcept{drop(p);} void operator delete[](void* p,std::align_val_t) noexcept{drop(p);}
void operator delete(void* p,size_t,std::align_val_t) noexcept{drop(p);} void operator delete[](void* p,size_t,std::align_val_t) noexcept{drop(p);}
void operator delete(void* p,const std::nothrow_t&) noexcept{drop(p);} void operator delete[](void* p,const std::nothrow_t&) noexcept{drop(p);}
void operator delete(void* p,std::align_val_t,const std::nothrow_t&) noexcept{drop(p);} void operator delete[](void* p,std::align_val_t,const std::nothrow_t&) noexcept{drop(p);}
extern "C" {
void* __real_malloc(size_t); void* __real_calloc(size_t,size_t); void* __real_realloc(void*,size_t); void __real_free(void*);
void* __wrap_malloc(size_t n){if(auto*c=activation_guard::watching())++c->cAlloc;return __real_malloc(n);}
void* __wrap_calloc(size_t n,size_t s){if(auto*c=activation_guard::watching())++c->cAlloc;return __real_calloc(n,s);}
void* __wrap_realloc(void*p,size_t n){if(auto*c=activation_guard::watching())++c->cAlloc;return __real_realloc(p,n);}
void __wrap_free(void*p){if(auto*c=activation_guard::watching())++c->cFree;__real_free(p);}
int __real_pthread_mutex_lock(pthread_mutex_t*);int __real_pthread_mutex_trylock(pthread_mutex_t*);
int __real_pthread_cond_wait(pthread_cond_t*,pthread_mutex_t*);
int __wrap_pthread_mutex_lock(pthread_mutex_t*p){if(auto*c=activation_guard::watching())++c->locks;return __real_pthread_mutex_lock(p);}
int __wrap_pthread_mutex_trylock(pthread_mutex_t*p){if(auto*c=activation_guard::watching())++c->locks;return __real_pthread_mutex_trylock(p);}
int __wrap_pthread_cond_wait(pthread_cond_t*p,pthread_mutex_t*m){if(auto*c=activation_guard::watching())++c->locks;return __real_pthread_cond_wait(p,m);}
ssize_t __real_read(int,void*,size_t);ssize_t __real_write(int,const void*,size_t);
FILE* __real_fopen(const char*,const char*);size_t __real_fwrite(const void*,size_t,size_t,FILE*);
ssize_t __wrap_read(int fd,void*p,size_t n){if(auto*c=activation_guard::watching())++c->io;return __real_read(fd,p,n);}
ssize_t __wrap_write(int fd,const void*p,size_t n){if(auto*c=activation_guard::watching())++c->io;return __real_write(fd,p,n);}
FILE* __wrap_fopen(const char*p,const char*m){if(auto*c=activation_guard::watching())++c->io;return __real_fopen(p,m);}
size_t __wrap_fwrite(const void*p,size_t s,size_t n,FILE*f){if(auto*c=activation_guard::watching())++c->io;return __real_fwrite(p,s,n,f);}
}
