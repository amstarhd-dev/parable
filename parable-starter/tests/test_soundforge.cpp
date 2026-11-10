#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include "lab_wav.h"
#include "test_check.h"
#include <array>
#include <limits>

template <typename F> void rejects(F action) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

int main() {
    // No RAT include, library, fixtures or firmware dependency in this target.
    using namespace soundforge;
    const auto alternating = stats({-1, 1, -1, 1});
    CHECK(alternating.rms == 1 && alternating.peak == 1 && alternating.mean == 0);
    const auto dc = stats({.25f, .25f});
    CHECK(dc.rms == .25 && dc.peak == .25 && dc.mean == .25);
    const auto sine = stats({0, 1, 0, -1});
    CHECK(std::abs(sine.rms-std::sqrt(.5)) < 1e-15 && sine.mean == 0);
    const auto silent = stats({0, 0});
    CHECK(silent.rms == 0 && silent.peak == 0 && silent.mean == 0);
    CHECK(db(0) == -300 && db(1) == 0 && std::abs(db(.1)+20) < 1e-14);
    rejects([] { stats({}); });
    for (float bad : {NAN, INFINITY, -INFINITY}) rejects([=] { stats({.1f, bad}); });
    for (double bad : {-1., double(NAN), double(INFINITY)}) rejects([=] { db(bad); });

    std::array<std::vector<float>, 3> signals{{{.5f, .5f, .5f, .5f}, {1, 0, 0, 0}, {0, 0, 0, 0}}};
    const auto original = signals;
    const double target = rmsTarget(signals, .5, .89);
    CHECK(target == .445 && signals == original); // A shared target, no hidden limiter.
    CHECK(rmsTarget(signals, .1, .89) == .1);
    for (unsigned i = 0; i < signals.size(); ++i) {
        const auto raw = stats(signals[i]);
        const double gain = rmsGain(raw, target);
        for (auto& x : signals[i]) x = static_cast<float>(x*gain);
        const auto result = stats(signals[i]);
        CHECK(result.peak <= .890001);
        CHECK(std::abs(result.rms-(i == 2 ? 0 : target)) < 1e-7);
    }
    CHECK(rmsGain(silent, target) == 1 && rmsGain(alternating, 0) == 0);
    CHECK(rmsTarget(std::array<std::vector<float>, 1>{{{0, 0}}}, .5, .89) == .5);
    rejects([] { rmsTarget(std::vector<std::vector<float>>{}, .5, .89); });
    rejects([] { rmsTarget(std::array<std::vector<float>, 1>{}, .5, .89); });
    for (double bad : {-1., double(NAN), double(INFINITY)}) {
        rejects([&] { rmsTarget(signals, bad, .89); });
        rejects([&] { rmsGain(alternating, bad); });
    }
    for (double bad : {0., -1., 1.1, double(NAN), double(INFINITY)})
        rejects([&] { rmsTarget(signals, .5, bad); });
    rejects([] { rmsGain({NAN, 1, 0}, .5); });
    rejects([] { rmsGain({1, INFINITY, 0}, .5); });
    rejects([] { rmsGain({1, 1, NAN}, .5); });
    rejects([] { rmsGain({std::numeric_limits<double>::min(), 1, 0},
                         std::numeric_limits<double>::max()); });
    const auto huge = stats({std::numeric_limits<float>::max()});
    CHECK(std::isfinite(huge.rms));

    // Reuse the existing parser, independently of the RAT laboratory.
    const lab::Wave pcm{48000, {0, .25f, -.5f}};
    const auto decoded = lab::decode(lab::encode(pcm));
    CHECK(decoded.rate == pcm.rate && decoded.samples == pcm.samples);
    const auto excitation = syntheticPlucks(.05);
    CHECK(excitation.rate == 48000 && excitation.samples.size() == 432000);
    CHECK(std::abs(stats(excitation.samples).peak-.05) < 1e-8);
    for (double bad : {0., -1., 1., double(NAN)}) rejects([&] { syntheticPlucks(bad); });
    std::cout << "Soundforge independent utility checks: " << testChecks << '\n';
}
