#pragma once
#include <cstddef>
#include <cstdint>
namespace resource_bench::tracking {
struct Metrics {uint64_t live=0,peak=0,allocations=0,callbackAllocations=0,callbackFrees=0,locks=0,io=0,overflow=0;};
extern bool enabled,audio;
extern Metrics metrics;
void start() noexcept;
}
