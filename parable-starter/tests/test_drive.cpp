#include "parable_drive.h"
#include "test_check.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>
#include <new>

using namespace parable::dsp;
static bool watching = false;
static std::size_t allocations = 0;
void* operator new(std::size_t n) {
    if (watching) ++allocations;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
static void bounded(float x) { CHECK(std::isfinite(x)); CHECK(std::fabs(x) <= 1.f); }
static float stimulus(int i) { return static_cast<float>(.7 * std::sin(i * .137) + .1 * std::cos(i * .57)); }
static void parameters() {
    Drive a, b;
    CHECK(a.configure(48000, {.5f,.8f,.3f,.8f}));
    CHECK(b.configure(48000, {.5f,.8f,.3f,.8f}));
    a.process(.2f); b.process(.2f);
    for (float bad : {NAN, INFINITY, -INFINITY}) {
        for (int field = 0; field < 5; ++field) {
            DriveParameters p{.5f,.8f,.3f,.8f}; float rate = 48000;
            switch (field) {
                case 0: rate = bad; break;
                case 1: p.drive = bad; break;
                case 2: p.tone = bad; break;
                case 3: p.bias = bad; break;
                case 4: p.level = bad; break;
            }
            CHECK(!a.configure(rate, p));
            CHECK(a.process(.1f) == b.process(.1f)); // Rejection preserves history.
        }
    }
    for (float bad : {-1.f, 0.f, 31999.f, 96001.f}) CHECK(!a.configure(bad));
    CHECK(!a.configure(48000, {}, static_cast<DriveRate>(99)));
    CHECK(a.configure(48000, {-99,99,-99,99}));
    const auto p = a.parameters();
    CHECK(p.drive == 0 && p.tone == 1 && p.bias == -1 && p.level == 1);
    CHECK(a.configure(48000, {99,-99,99,-99}));
    CHECK(a.parameters().drive == 1 && a.parameters().tone == 0);
    CHECK(a.parameters().bias == 1 && a.parameters().level == 0);
}
static void sampleAccurate() {
    std::array<float, 1027> input{}, perSample{}, block{};
    for (std::size_t i = 0; i < input.size(); ++i) input[i] = stimulus(static_cast<int>(i));
    for (auto mode : {DriveRate::Oversampled4x, DriveRate::Reference1x}) {
        Drive a, b;
        CHECK(a.configure(44100, {}, mode)); CHECK(b.configure(44100, {}, mode));
        for (std::size_t i = 0; i < input.size(); ++i) perSample[i] = a.process(input[i]);
        block = input;
        b.processBlock(block.data(), block.data(), 0);
        b.processBlock(block.data(), block.data(), 13);
        b.processBlock(block.data()+13, block.data()+13, block.size()-13);
        CHECK(block == perSample); // Exact sample/block and in-place identity.
        b.reset(); b.processBlock(input.data(), block.data(), input.size());
        CHECK(block == perSample); // Exact repeatability after reset.
        for (int i = 0; i < 51; ++i) {
            CHECK(a.process(NAN) == 0); CHECK(a.process(INFINITY) == 0);
            CHECK(a.process(-INFINITY) == 0);
            CHECK(a.process(.1f) == b.process(.1f));
        }
    }
}
static void stability() {
    const float max = std::numeric_limits<float>::max();
    const float inputs[] = {max, -max, 0.f, -0.f, std::numeric_limits<float>::denorm_min(),
        std::numeric_limits<float>::min(), NAN, INFINITY, -INFINITY, .1f};
    uint32_t seed = 0x50415236;
    for (float rate : {32000.f,44100.f,48000.f,96000.f}) {
        for (float bias : {-1.f,0.f,1.f}) {
            Drive a;
            CHECK(a.configure(rate, {1,1,bias,1}));
            watching = true;
            for (int i = 0; i < 4096; ++i) {
                bounded(a.process(inputs[i % 10]));
                seed = seed * 1664525u + 1013904223u;
                float input; std::memcpy(&input, &seed, sizeof(input));
                bounded(a.process(input));
            }
            watching = false;
            CHECK(allocations == 0);
            for (int i = 0; i < static_cast<int>(rate * 3); ++i) bounded(a.process(.5f));
            CHECK(std::fabs(a.process(.5f)) < 1e-6f); // Constant input/DC rejection.
            for (int i = 0; i < static_cast<int>(rate * 3); ++i) bounded(a.process(0));
            CHECK(a.process(0) == 0); // Silence tail reaches exact zero.
            a.reset();
            for (int i = 0; i < 256; ++i) CHECK(a.process(0) == 0);
            CHECK(a.process(std::numeric_limits<float>::denorm_min()) == 0);
            CHECK(a.process(std::numeric_limits<float>::min()) == 0);
            CHECK(a.configure(rate, {1,1,bias,0}));
            for (int i = 0; i < 256; ++i) CHECK(a.process(stimulus(i)) == 0);
        }
    }
}
static void nonlinearAndShaping() {
    Drive low, high, symmetric, asymmetric, dark, bright, small, large;
    CHECK(low.configure(48000, {0,1,0,1})); CHECK(high.configure(48000, {1,1,0,1}));
    CHECK(symmetric.configure(48000, {.8f,1,0,1}));
    CHECK(asymmetric.configure(48000, {.8f,1,1,1}));
    CHECK(dark.configure(48000, {.5f,0,0,1})); CHECK(bright.configure(48000, {.5f,1,0,1}));
    CHECK(small.configure(48000, {.8f,1,0,1})); CHECK(large.configure(48000, {.8f,1,0,1}));
    double driveDifference = 0, biasDifference = 0, darkEnergy = 0, brightEnergy = 0;
    double smallEnergy = 0, largeEnergy = 0;
    for (int i = 0; i < 12000; ++i) {
        const float x = static_cast<float>(.3 * std::sin(2 * 3.141592653589793 * 1000 * i / 48000));
        const float a = low.process(x), b = high.process(x);
        const float c = symmetric.process(x), d = asymmetric.process(x);
        const float e = dark.process(x), f = bright.process(x);
        const float g = small.process(x * .02f), h = large.process(x);
        if (i < 2400) continue;
        driveDifference += (a-b)*(a-b); biasDifference += (c-d)*(c-d);
        darkEnergy += e*e; brightEnergy += f*f; smallEnergy += g*g; largeEnergy += h*h;
    }
    CHECK(driveDifference > 100); CHECK(biasDifference > 1);
    CHECK(brightEnergy > darkEnergy * 2);
    CHECK(largeEnergy > smallEnergy * 5); // More input still produces more output.
    CHECK(largeEnergy < smallEnergy * 50 * 50 * .5); // Nonlinear compression, not scaled gain.
}
int main() {
    parameters(); sampleAccurate(); stability(); nonlinearAndShaping();
    std::cout << "PAR006 drive passed: " << testChecks << " checks, processing allocations " << allocations << '\n';
}
