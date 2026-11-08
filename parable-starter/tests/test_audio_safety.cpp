#include "parable_core.h"
#include "test_check.h"
#include <cstring>
#include <limits>

using namespace parable;
constexpr float kRate = 48000.f;
constexpr float kTolerance = 2e-5f;

static void bounded(float sample) {
    CHECK(std::isfinite(sample));
    CHECK(sample >= -1.f && sample <= 1.f);
}

static Patch gainChain() {
    Patch p;
    p.count = kMaxStages;
    for (auto& stage : p.stages) stage = {Type::Gain, 2.f, true};
    return p;
}

static void overflowRegression() {
    Patch p = gainChain();
    p.outputTrim = 0.f;
    Engine engine(kRate);
    CHECK(engine.load(p) == Status::Ok);
    const float sample = engine.process(std::numeric_limits<float>::max());
    bounded(sample); // PAR-001 reproducer: fails on the original engine, also in Release.
    CHECK(sample == 0.f);
}

static void extremeSequences() {
    const float max = std::numeric_limits<float>::max();
    const float inputs[] = {max, -max, max / 2.f, -max / 2.f, 0.f, -0.f,
        std::numeric_limits<float>::min(), std::numeric_limits<float>::denorm_min(),
        NAN, INFINITY, -INFINITY, .25f, -.5f};
    for (bool filter : {false, true}) {
        Patch p = gainChain();
        if (filter) p.stages[kMaxStages - 1] = {Type::OnePoleLowpass, 1000.f, true};
        for (float trim : {0.f, .5f, 1.f}) {
            p.outputTrim = trim;
            Engine engine(kRate);
            CHECK(engine.load(p) == Status::Ok);
            for (float volume : {0.f, .5f, 1.f}) {
                engine.setVolume(volume);
                for (int repeat = 0; repeat < 64; ++repeat) {
                    for (float input : inputs) {
                        const float y = engine.process(input);
                        bounded(y);
                        if (trim == 0.f || volume == 0.f || !finite(input)) CHECK(y == 0.f);
                    }
                }
            }
        }
    }

    // Alternating extremes used to overflow x - state in a filter even without gains.
    // Check recovery at the slowest supported cutoff and several sample rates.
    for (float rate : {8000.f, kRate, 192000.f}) {
        for (float cutoff : {20.f, .44f * rate}) {
            Patch p;
            p.count = 1;
            p.stages[0] = {Type::OnePoleLowpass, cutoff, true};
            Engine engine(rate), fresh(rate);
            CHECK(engine.load(p) == Status::Ok);
            CHECK(fresh.load(p) == Status::Ok);
            for (int i = 0; i < 128; ++i) bounded(engine.process(i % 2 ? -max : max));
            for (int i = 0; i < 65536; ++i) bounded(engine.process(0.f));
            CHECK(std::fabs(engine.process(.25f) - fresh.process(.25f)) < kTolerance);
        }
    }
}

static void rejectedInputAndPatchRecovery() {
    Patch p;
    p.count = 2;
    p.stages[0] = {Type::Gain, 1.5f, true};
    p.stages[1] = {Type::OnePoleLowpass, 1000.f, true};
    Engine engine(kRate), reference(kRate);
    CHECK(engine.load(p) == Status::Ok);
    CHECK(reference.load(p) == Status::Ok);
    CHECK(engine.process(.5f) == reference.process(.5f));
    // A non-finite sample is discarded without advancing/poisoning filter history.
    for (float bad : {NAN, INFINITY, -INFINITY}) {
        CHECK(engine.process(bad) == 0.f);
        CHECK(engine.process(.25f) == reference.process(.25f));
    }
    for (int scenario = 0; scenario < 8; ++scenario) {
        Patch bad = p;
        Status expected = Status::InvalidParameter;
        switch (scenario) {
            case 0: bad.schema = 999; expected = Status::SchemaMismatch; break;
            case 1: bad.count = kMaxStages + 1; expected = Status::TooManyStages; break;
            case 2: bad.outputTrim = NAN; break;
            case 3: bad.outputTrim = INFINITY; break;
            case 4: bad.stages[0].parameter = -1.f; break;
            case 5: bad.stages[1].parameter = INFINITY; break;
            case 6: bad.stages[1].parameter = .45f * kRate; break;
            case 7: bad.stages[0].type = static_cast<Type>(99);
                    expected = Status::InvalidType; break;
        }
        CHECK(engine.load(bad) == expected);
        CHECK(engine.loaded().schema == p.schema);
        CHECK(engine.loaded().count == p.count);
        CHECK(engine.loaded().outputTrim == p.outputTrim);
        for (float input : {0.f, -.2f, .4f})
            CHECK(engine.process(input) == reference.process(input));
    }
    // Invalid volume values mute safely; a subsequent valid value restores playback.
    for (float volume : {NAN, INFINITY, -INFINITY, -1.f, 0.f, -0.f}) {
        engine.setVolume(volume);
        CHECK(engine.process(std::numeric_limits<float>::max()) == 0.f);
    }
    engine.setVolume(1.f);
    for (int i = 0; i < 1024; ++i) bounded(engine.process(0.f));
    CHECK(engine.process(.25f) > 0.f);
}

static void ordinaryInput() {
    // Independent double-precision reference: original unsaturated processors,
    // convex-combination filter equation, and only the existing final output clamp.
    for (float rate : {8000.f, kRate, 192000.f}) {
        for (int layout = 0; layout < 4; ++layout) {
            Patch p = layout == 0 ? makeBypassPatch() : gainChain();
            if (layout == 2) {
                p.stages[1] = {Type::OnePoleLowpass, 20.f, true};
                p.stages[4] = {Type::OnePoleLowpass, .44f * rate, true};
                p.stages[6] = {Type::Gain, .01f, true};
            } else if (layout == 3) {
                p.stages[2] = {Type::Gain, 0.f, false};
                p.stages[3] = {Type::Bypass, 1.f, true};
                p.stages[5] = {Type::OnePoleLowpass, 1000.f, false};
            }
            for (float trim : {0.f, .125f, 1.f}) {
                p.outputTrim = trim;
                for (float volume : {0.f, .3f, 1.f}) {
                    Engine engine(rate);
                    CHECK(engine.load(p) == Status::Ok);
                    engine.setVolume(volume);
                    std::array<double, kMaxStages> state{};
                    const float edges[] = {1.f, -1.f, 0.f, -0.f, .5f, -.25f};
                    for (int i = 0; i < 1024; ++i) {
                        const float input = i < 6 ? edges[i] : static_cast<float>(std::sin(i * .137));
                        double expected = input;
                        for (size_t j = 0; j < p.count; ++j) {
                            const auto& s = p.stages[j];
                            if (!s.enabled) continue;
                            if (s.type == Type::Gain) expected *= s.parameter;
                            if (s.type == Type::OnePoleLowpass) {
                                const double a = 1. - std::exp(-6.283185307179586 * s.parameter / rate);
                                state[j] = (1. - a) * state[j] + a * expected;
                                expected = state[j];
                            }
                        }
                        expected = std::fmax(-1., std::fmin(1., expected * trim * volume));
                        const float y = engine.process(input);
                        bounded(y);
                        CHECK(std::fabs(y - expected) < kTolerance);
                    }
                }
            }
        }
    }
}

static uint32_t randomBits(uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return seed;
}

static void generatedInvariants() {
    static_assert(sizeof(float) == sizeof(uint32_t) && std::numeric_limits<float>::is_iec559,
                  "Bit-pattern input coverage requires IEEE-754 binary32");
    uint32_t seed = 0x50415233u;
    for (int fixture = 0; fixture < 128; ++fixture) {
        const float rate = fixture % 2 ? kRate : 192000.f;
        Patch p;
        p.count = randomBits(seed) % (kMaxStages + 1);
        p.outputTrim = fixture % 3 ? 1.f : 0.f;
        for (size_t i = 0; i < p.count; ++i) {
            auto& s = p.stages[i];
            s.type = static_cast<Type>(randomBits(seed) % 3);
            s.enabled = (randomBits(seed) & 4u) != 0;
            const float unit = static_cast<float>(randomBits(seed) % 1025) / 1024.f;
            s.parameter = s.type == Type::OnePoleLowpass ? 20.f + unit * (.44f * rate - 20.f) : 2.f * unit;
        }
        Engine engine(rate);
        CHECK(engine.load(p) == Status::Ok);
        for (int sample = 0; sample < 1024; ++sample) {
            const uint32_t bits = randomBits(seed);
            float input;
            std::memcpy(&input, &bits, sizeof(input));
            const float y = engine.process(input);
            bounded(y);
            if (!finite(input) || p.outputTrim == 0.f) CHECK(y == 0.f);
        }
        for (int sample = 0; sample < 32; ++sample) bounded(engine.process(0.f));
    }
}

int main() {
    overflowRegression();
    extremeSequences();
    rejectedInputAndPatchRecovery();
    ordinaryInput();
    generatedInvariants();
    std::cout << "PAR003 audio safety passed: " << testChecks << " checks\n";
}
