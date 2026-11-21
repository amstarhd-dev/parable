#include "host_tracker.h"
#include <cstdio>
#include <cstdlib>
#include <new>
#include <pthread.h>
extern "C" void* __wrap_malloc(size_t);
extern "C" void __wrap_free(void*);
int main(){using namespace resource_bench::tracking;
 start();void* a=__wrap_malloc(73);if(!a||metrics.live!=73||metrics.peak!=73||metrics.allocations!=1)return 1;
 void* b=::operator new(31);if(metrics.live!=104||metrics.peak!=104)return 2;
 __wrap_free(a);if(metrics.live!=31)return 3;::operator delete(b);if(metrics.live)return 4;
 FILE* f=std::tmpfile();if(!f)return 5;pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
 audio=true;void* c=__wrap_malloc(17);__wrap_free(c);pthread_mutex_lock(&lock);pthread_mutex_unlock(&lock);std::fwrite("x",1,1,f);audio=false;
 if(metrics.callbackAllocations!=1||metrics.callbackFrees!=1||metrics.locks!=1||metrics.io!=1||metrics.overflow)return 6;
 enabled=false;std::fclose(f);return 0;
}
