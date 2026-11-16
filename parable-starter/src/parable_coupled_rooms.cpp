// Mixed-feedback/Householder pattern adapted from Signalsmith Basics reverb.h,
// Copyright2022-2025 Signalsmith Audio/Geraint Luff, MIT. See retained notice.
// Dual-room partition, contraction connection, fixed storage and controls: Parable.
#include "parable_coupled_rooms.h"
#include <algorithm>
#include <cmath>

namespace parable::soundforge {
namespace {
constexpr float times[8] = {.0113f,.0137f,.0179f,.0197f,.0293f,.0371f,.0437f,.0531f};
bool rateValid(float r) noexcept { return r == 44100 || r == 48000 || r == 96000; }
float flush(float v) noexcept { return std::abs(v) < 1.e-25f ? 0.f : v; }
}
std::size_t CoupledRooms::requiredSamples(float rate) noexcept {
    if (!rateValid(rate)) return 0;
    std::size_t n = 0;
    for (float t : times) n += static_cast<unsigned>(t*rate+.5f);
    return n;
}
bool CoupledRooms::valid(RoomControls c) noexcept {
    return std::isfinite(c.aperture) && c.aperture >= 0 && c.aperture <= .45f
        && std::isfinite(c.wet) && c.wet >= 0 && c.wet <= 1;
}
bool CoupledRooms::prepare(float rate, float* storage, std::size_t count, RoomControls c) noexcept {
    const auto needed = requiredSamples(rate);
    if (!needed || !storage || count < needed || !valid(c)) return false;
    data_ = storage; count_ = needed; target_ = c;
    std::size_t offset = 0;
    for (unsigned i = 0; i < 8; ++i) {
        lengths_[i] = static_cast<unsigned>(times[i]*rate+.5f);
        offsets_[i] = static_cast<unsigned>(offset); offset += lengths_[i];
        losses_[i] = std::pow(.001f, (lengths_[i]/rate)/(i < 4 ? .65f : 1.9f));
    }
    poles_[0] = std::exp(-6.28318530718f*5500/rate);
    poles_[1] = std::exp(-6.28318530718f*3200/rate);
    smooth_ = 1-std::exp(-1/(.02f*rate));
    reset(); return true;
}
void CoupledRooms::reset() noexcept {
    if (data_) std::fill_n(data_,count_,0.f);
    for (unsigned i = 0; i < 8; ++i) { positions_[i] = 0; damping_[i] = 0; }
    aperture_ = target_.aperture; polarity_ = target_.invertedReturn ? -1.f : 1.f;
    wet_ = target_.wet;
}
bool CoupledRooms::setControls(RoomControls c) noexcept {
    if (!valid(c)) return false;
    target_ = c; return true;
}
RoomOutput CoupledRooms::process(float inputA, float inputB) noexcept {
    if (!data_ || !std::isfinite(inputA) || !std::isfinite(inputB)) return {};
    inputA = std::clamp(inputA,-1.f,1.f); inputB = std::clamp(inputB,-1.f,1.f);
    aperture_ += smooth_*(target_.aperture-aperture_);
    polarity_ += smooth_*((target_.invertedReturn ? -1.f : 1.f)-polarity_);
    wet_ += smooth_*(target_.wet-wet_);
    float v[8], mixed[8];
    for (unsigned i = 0; i < 8; ++i) {
        const float raw = data_[offsets_[i]+positions_[i]];
        damping_[i] = flush((1-poles_[i/4])*raw+poles_[i/4]*damping_[i]);
        v[i] = damping_[i];
    }
    for (unsigned room = 0; room < 2; ++room) {
        const unsigned k = 4*room;
        const float halfSum = .5f*(v[k]+v[k+1]+v[k+2]+v[k+3]);
        for (unsigned j = 0; j < 4; ++j) mixed[k+j] = v[k+j]-halfSum;
    }
    const float returnA = mixed[0], returnB = mixed[4];
    mixed[0] = (1-aperture_)*returnA + polarity_*aperture_*returnB;
    mixed[4] = aperture_*returnA + (1-aperture_)*returnB;
    for (unsigned i = 0; i < 8; ++i) {
        const float excite = (i < 4 ? inputA : inputB)*.5f;
        data_[offsets_[i]+positions_[i]] = flush(excite+losses_[i]*mixed[i]);
        if (++positions_[i] == lengths_[i]) positions_[i] = 0;
    }
    const float a = .5f*(v[0]-v[1]+v[2]+v[3]);
    const float b = .5f*(v[4]-v[5]+v[6]+v[7]);
    return {(1-wet_)*inputA+wet_*.5f*(a+b), a, b};
}
} // namespace parable::soundforge
