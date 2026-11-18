#pragma once
#include <array>
#include <cstddef>
namespace parable::soundforge {
struct TiledSettings { float decaySeconds=.62f, wet=.35f; bool ceramic=false; };
struct TiledOutput { float left=0,right=0,mono=0,wetLeft=0,wetRight=0; };
// Standalone fixed-topology specimen, not an Engine/patch processor registration.
// Caller retains storage. prepare/reset are stopped-only; one audio owner.
class TiledRoom {
public:
    TiledRoom()=default;
    TiledRoom(const TiledRoom&)=delete;TiledRoom& operator=(const TiledRoom&)=delete;
    static constexpr size_t kMaxSamples=24576;
    static size_t requiredSamples(float rate) noexcept;
    bool prepare(float rate,float* storage,size_t count,TiledSettings={}) noexcept;
    void reset() noexcept;
    TiledOutput process(float input) noexcept;
    size_t bufferBytes() const noexcept {return used_*sizeof(float);}
    float loopGain() const noexcept {return gain_;}
    float diffuserGain() const noexcept {return settings_.ceramic?.94f:apGain_;}
private:
    struct Delay {unsigned offset=0,length=0,write=0;float frames=0;};
    std::array<Delay,13> delays_{};
    std::array<float,7> leftTaps_{},rightTaps_{};
    float* data_=nullptr;size_t used_=0;TiledSettings settings_{};
    float gain_=0,apGain_=0,inputA_=0,inputB_=0,dampA_=0,dampB_=0;
    float inputState_=0,leftState_=0,rightState_=0,leftOut_=0,rightOut_=0;
    bool prepared_=false;
    static std::array<float,13> times(float rate) noexcept;
    float tap(unsigned line,float frames) const noexcept;
    void push(unsigned line,float value) noexcept;
    float delay(unsigned line,float value) noexcept;
    float allpass(unsigned line,float value,float gain) noexcept;
};
}
