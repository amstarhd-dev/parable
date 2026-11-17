#pragma once
#include <array>
namespace wallace {
struct RoutingConfig {
    float core=.39f, clean=0, rotary=.21f, tape=.20f, hall=.20f;
    float sendRotary=1,sendTape=1,sendHall=1;
    float tapeHz=120,hallHz=180,attackMs=10,releaseMs=250,duckDb=4;
    float low=.012f,high=.05f;
    bool carving=true,ducking=true;
};
struct Returns {
    float output=0,wet=0,rotary=0,tape=0,hall=0,core=0,clean=0,envelope=0,gain=1;
};
// Only the Wallace experiment's fixed return mixer, not a general graph/Engine.
class Routing {
public:
    bool prepare(const RoutingConfig&) noexcept; // stopped, fixed48k
    Returns process(float input,float fuzz,float rotary,float tape,float hall) noexcept;
private:
    struct Highpass {
        float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        void prepare(float hz) noexcept;
        float process(float x) noexcept;
    } tape_,hall_;
    RoutingConfig config_{};
    std::array<float,48> cleanDelay_{};
    unsigned position_=0;
    float envelope_=0,attack_=0,release_=0,minGain_=1;
};
}
