#pragma once
#include <cstddef>
namespace activation_guard {
struct Counts { size_t cppAlloc=0, cppFree=0, cAlloc=0, cFree=0, bytes=0, locks=0, io=0; };
extern thread_local bool callback, preparation;
extern thread_local Counts audio, control;
extern thread_local size_t failAfter;
void selfTest();
inline void begin() noexcept { callback=true; }
inline void end() noexcept { callback=false; }
inline bool clean() noexcept { return !audio.cppAlloc && !audio.cppFree && !audio.cAlloc && !audio.cFree && !audio.locks && !audio.io; }
}
