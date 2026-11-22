#pragma once

#include <algorithm>
#include <cmath>
#include "coefficients.h"

namespace parable::dsp {

// 48 kHz mono. Sustain operates on the fitted internal nonlinear branches,
// Tone on the loaded RC model, and Volume at the output divider.
class ReducedMuff {
public:
    explicit ReducedMuff(bool highR15 = false) noexcept : fault_(highR15) {
        updateCoefficients();
    }

    // Call from the thread that owns process(). Values are electrical fractions.
    void setControls(float sustain, float tone, float volume) noexcept {
        if (std::isfinite(sustain)) sustainTarget_ = std::clamp(sustain, 0.f, 1.f);
        if (std::isfinite(tone)) toneTarget_ = std::clamp(tone, 0.f, 1.f);
        if (std::isfinite(volume)) volumeTarget_ = std::clamp(volume, 0.f, 1.f);
    }

    float process(float input) noexcept {
        using namespace muff_control_detail;
        if (!std::isfinite(input)) input = 0;
        input = std::clamp(input, -1.f, 1.f);
        if (!controlCountdown_) {
            if (sustain_ != sustainTarget_ || tone_ != toneTarget_) {
                sustain_ = approach(sustain_, sustainTarget_, .0645f);
                tone_ = approach(tone_, toneTarget_, .0645f);
                updateCoefficients();
            }
            controlCountdown_ = 32;
        }
        --controlCountdown_;

        float features[14] = {input, input - lastInput_};
        lastInput_ = input;
        for (unsigned i = 0; i < 4; ++i) {
            inputLow_[i] += (1 - inPole[i]) * (input - inputLow_[i]);
            features[2 + i] = inputLow_[i];
            features[6 + i] = input - inputLow_[i];
        }
        for (unsigned i = 0; i < 2; ++i) {
            rectifiedLow_[i] += (1 - inPole[i]) * (std::abs(input) - rectifiedLow_[i]);
            clippedLow_[i] += (1 - inPole[i]) * (std::clamp(10 * input, -1.f, 1.f) - clippedLow_[i]);
            features[10 + i] = rectifiedLow_[i];
            features[12 + i] = clippedLow_[i];
        }

        float buses[4]{};
        for (const auto& branch : branches_) {
            float argument = branch.bias;
            for (unsigned i = 0; i < 14; ++i)
                argument += branch.weight[i] * features[i];
            const float response = saturate(argument) - branch.offset;
            for (unsigned i = 0; i < 4; ++i)
                buses[i] += branch.readout[i] * response;
        }
        for (unsigned i = 0; i < 3; ++i) {
            postLow_[i] += (1 - outPole[i]) * (buses[i + 1] - postLow_[i]);
            buses[0] += postLow_[i];
        }

        const float filtered = toneB_[0] * buses[0] + toneState_[0];
        toneState_[0] = toneB_[1] * buses[0] - toneA_[0] * filtered + toneState_[1];
        toneState_[1] = toneB_[2] * buses[0] - toneA_[1] * filtered + toneState_[2];
        toneState_[2] = toneB_[3] * buses[0] - toneA_[2] * filtered;

        volume_ = approach(volume_, volumeTarget_, .0035f);
        const float delayed = delay_[delayIndex_];
        delay_[delayIndex_] = filtered;
        if (++delayIndex_ == 48) delayIndex_ = 0;
        // 100k Volume and 1M output load, normalized to its noon response.
        const float gain = volume_ / (1 + .1f * volume_ * (1 - volume_));
        return std::clamp(delayed * (2.05f * gain), -1.f, 1.f);
    }

private:
    struct Branch {
        float weight[14]{}, readout[4]{}, bias = 0, offset = 0;
    };
    Branch branches_[6]{};
    bool fault_;
    unsigned controlCountdown_ = 0, delayIndex_ = 0;
    float sustain_ = .75f, tone_ = .5f, volume_ = .5f;
    float sustainTarget_ = .75f, toneTarget_ = .5f, volumeTarget_ = .5f;
    float inputLow_[4]{}, rectifiedLow_[2]{}, clippedLow_[2]{}, postLow_[3]{};
    float toneB_[4]{}, toneA_[3]{}, toneState_[3]{}, delay_[48]{}, lastInput_ = 0;

    static float approach(float value, float target, float rate) noexcept {
        const float delta = target - value;
        return std::abs(delta) < 1e-6f ? target : value + delta * rate;
    }

    static float saturate(float x) noexcept {
        const float scaled = std::clamp(x * 64.f + 512.f, 0.f, 1024.f);
        const unsigned index = std::min(unsigned(scaled), 1023u);
        const auto& table = muff_control_detail::activation;
        return table[index] + (scaled - index) * (table[index + 1] - table[index]);
    }

    void updateCoefficients() noexcept {
        using namespace muff_control_detail;
        const float sustain = 2 * sustain_ - 1;
        for (unsigned j = 0; j < 6; ++j) {
            auto& branch = branches_[j];
            branch.bias = bias[fault_][0][j] + sustain * bias[fault_][1][j];
            branch.offset = saturate(branch.bias);
            for (unsigned i = 0; i < 14; ++i)
                branch.weight[i] = weight[fault_][0][j][i] + sustain * weight[fault_][1][j][i];
            for (unsigned i = 0; i < 4; ++i)
                branch.readout[i] = readout[fault_][0][j][i] + sustain *
                                    (readout[fault_][1][j][i] + sustain * readout[fault_][2][j][i]);
        }
        const float position = tone_ * 64;
        const unsigned index = std::min(unsigned(position), 63u);
        const float blend = position - index;
        for (unsigned i = 0; i < 7; ++i) {
            const float value = tone[index][i] + blend * (tone[index + 1][i] - tone[index][i]);
            (i < 4 ? toneB_[i] : toneA_[i - 4]) = value;
        }
    }
};
}
