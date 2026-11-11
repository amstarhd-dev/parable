#include "parable_prepare.h"
#include "prepare_internal.h"
#include "test_check.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <new>

using namespace parable;
namespace data = parable::patch;
namespace prep = parable::preparation;
using Json = nlohmann::json;
static bool watching = false;
static size_t allocations = 0;
void* operator new(size_t n) {
    if (watching) ++allocations;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t n) { return ::operator new(n); }
#if defined(__GNUC__)
__attribute__((noinline))
#endif
static void release(void* p) noexcept { std::free(p); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, size_t) noexcept { release(p); }
void operator delete[](void* p, size_t) noexcept { release(p); }

static std::string fixture(const std::string& name) {
    std::ifstream f(std::string(PATCH_FIXTURES) + "/" + name, std::ios::binary);
    CHECK(f.is_open());
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static data::ValidatedPatch decode(const std::string& text) {
    const auto result = data::Codec::decode(text);
    if (const auto* e = std::get_if<data::Error>(&result)) std::cerr << e->field << ": " << e->rule << '\n';
    CHECK(std::holds_alternative<data::ValidatedPatch>(result));
    return std::get<data::ValidatedPatch>(result);
}
static Patch native(const data::ValidatedPatch& document, float rate = 48000) {
    const auto result = prep::prepare(document, rate);
    CHECK(std::holds_alternative<Patch>(result));
    return std::get<Patch>(result);
}
static void reject(const Json& json, data::ErrorCode code, const std::string& field) {
    const auto result = data::Codec::decode(json.dump());
    CHECK(std::holds_alternative<data::Error>(result));
    const auto& e = std::get<data::Error>(result);
    CHECK(e.code == code); CHECK(e.field == field); CHECK(!e.rule.empty());
}
static float signal(size_t n) {
    return n % 173 == 0 ? .15f : static_cast<float>(.09 * std::sin(.071 * n) + .04 * std::sin(.197 * n));
}
static void bounded(float x) { CHECK(std::isfinite(x)); CHECK(std::abs(x) <= 1); }

static void contract() {
    for (const std::string name : {"rat", "rat-composite", "rat-reversed"}) {
        const auto d = decode(fixture(name + ".json"));
        CHECK(data::Codec::encode(d) == fixture(name + ".canonical.json"));
        CHECK(decode(data::Codec::encode(d)) == d);
        CHECK(data::Codec::encode(decode(fixture(name + ".canonical.json"))) == fixture(name + ".canonical.json"));
    }
    const auto a = decode(fixture("rat-composite.json")), b = decode(fixture("rat-reversed.json"));
    CHECK(a.processors() == b.processors());
    CHECK(a.serialOrder()[0] == 40 && a.serialOrder()[1] == 7 && a.serialOrder()[2] == 3);
    CHECK(b.serialOrder()[0] == 3 && b.serialOrder()[1] == 7 && b.serialOrder()[2] == 40);
    const auto n = native(a);
    CHECK(n.count == 3 && n.stages[0].type == Type::Gain && n.stages[1].type == Type::Rat
          && n.stages[2].type == Type::OnePoleLowpass);
    CHECK(n.stages[1].rat.distortion == .35f && n.stages[1].rat.filter == .5f && n.stages[1].rat.volume == .7f);
    auto j = Json::parse(fixture("rat.json"));
    for (const std::string control : {"distortion", "filter", "volume"}) {
        for (double value : {-.01, 1.01}) {
            auto bad = j; bad["processors"][0]["parameters"][control]["value"] = value;
            reject(bad, data::ErrorCode::OutOfRange, "$/processors/0/parameters/" + control + "/value");
        }
        auto bad = j; bad["processors"][0]["parameters"][control]["unit"] = "linear";
        reject(bad, data::ErrorCode::InvalidUnit, "$/processors/0/parameters/" + control + "/unit");
        bad = j; bad["processors"][0]["parameters"].erase(control);
        reject(bad, data::ErrorCode::MissingField, "$/processors/0/parameters/" + control);
        for (const char* token : {"NaN", "Infinity", "1e400", "1e-100"}) {
            auto badText = j.dump();
            const auto value = badText.find("\"value\":", badText.find("\"" + control + "\"")) + 8;
            const auto end = badText.find('}', value);
            badText.replace(value, end - value, token);
            CHECK(std::holds_alternative<data::Error>(data::Codec::decode(badText)));
        }
    }
    auto bad = j; bad["processors"][0]["parameters"]["circuit"] = Json::object();
    reject(bad, data::ErrorCode::UnknownField, "$/processors/0/parameters/circuit");
    bad = j; bad["processors"][0]["revision"] = 2;
    reject(bad, data::ErrorCode::UnsupportedVersion, "$/processors/0/revision");
    bad = j; bad["version"] = 2;
    reject(bad, data::ErrorCode::UnsupportedVersion, "$/version");
    bad = j; bad["routing"][0]["to"] = 88;
    reject(bad, data::ErrorCode::InvalidEndpoint, "$/routing/0/to");
    for (bool enabled : {true, false}) {
        bad = j; auto second = bad["processors"][0]; second["id"] = 8; second["enabled"] = enabled;
        bad["processors"].push_back(second);
        reject(bad, data::ErrorCode::LimitExceeded, "$/processors/1/type");
    }
    for (float rate : {8000.f, 31999.f, 96001.f, 192000.f}) {
        bad = j; bad["sampleRate"]["value"] = rate;
        reject(bad, data::ErrorCode::OutOfRange, "$/sampleRate/value");
    }
    // Existing utility capacity/rates remain supported, including eight stages.
    for (float rate : {8000.f, 192000.f}) {
        Patch p; p.count = kMaxStages;
        for (auto& s : p.stages) s = {Type::Gain, .5f, true};
        Engine e(rate); CHECK(e.load(p) == Status::Ok); CHECK(e.process(.5f) == .5f / 256);
    }
}

static void audioAndOrdering() {
    const auto doc = decode(fixture("rat.json"));
    Engine engine(48000); CHECK(engine.load(native(doc)) == Status::Ok);
    dsp::Rat reference; CHECK(reference.configure(48000, {.35f, .5f, .7f}));
    for (size_t i = 0; i < 8192; ++i) {
        const float x = i == 10 ? 200.f : signal(i);
        const float want = reference.process(std::clamp(x, -4.f, 4.f)) * .8f;
        CHECK(engine.process(x) == want);
    }
    Engine composite(48000), reversed(48000);
    CHECK(composite.load(native(decode(fixture("rat-composite.json")))) == Status::Ok);
    CHECK(reversed.load(native(decode(fixture("rat-reversed.json")))) == Status::Ok);
    CHECK(reference.configure(48000, {.35f, .5f, .7f}));
    const float alpha = 1.f - std::exp(-6.283185307179586f * 1800.f / 48000.f);
    float history = 0; double difference = 0;
    watching = true;
    for (size_t i = 0; i < 16384; ++i) {
        const float x = signal(i);
        const float rat = reference.process(x * 1.8f);
        history += alpha * (rat - history);
        const float y = composite.process(x), other = reversed.process(x);
        CHECK(y == history * .8f); // Independent manual Gain -> native Rat -> LP -> trim.
        bounded(y); bounded(other);
        difference += double(y - other) * (y - other);
    }
    watching = false; CHECK(allocations == 0);
    CHECK(std::sqrt(difference / 16384) > .001);
    // Disabled RAT is exact dry pass, and diagnostic circuit history never advances.
    auto disabled = native(doc); disabled.stages[0].enabled = false;
    CHECK(engine.load(disabled) == Status::Ok);
    for (size_t i = 0; i < 4096; ++i) CHECK(engine.process(signal(i)) == signal(i) * .8f);
    const auto nodes = engine.ratNodes();
    CHECK(nodes.linearOpamp == 0 && nodes.opamp == 0 && nodes.clip == 0 && nodes.tone == 0 && nodes.jackVolts == 0);
    // Reload resets FIR, circuit history and downstream utility states.
    CHECK(engine.load(native(doc)) == Status::Ok); CHECK(reference.configure(48000, {.35f, .5f, .7f}));
    for (size_t i = 0; i < 4096; ++i) CHECK(engine.process(signal(i)) == reference.process(signal(i)) * .8f);
}

static void failuresAndPresets() {
    const auto document = decode(fixture("rat-composite.json"));
    const auto p = native(document);
    Engine current(48000), unchanged(48000);
    CHECK(current.load(p) == Status::Ok); CHECK(unchanged.load(p) == Status::Ok);
    for (size_t i = 0; i < 1024; ++i) CHECK(current.process(signal(i)) == unchanged.process(signal(i)));
    for (int fault = 0; fault < 6; ++fault) {
        auto bad = p;
        switch (fault) {
            case 0: bad.stages[1].rat.distortion = NAN; break;
            case 1: bad.stages[1].rat.filter = INFINITY; break;
            case 2: bad.stages[1].rat.volume = 1.01f; break;
            case 3: bad.stages[0] = bad.stages[1]; break;
            case 4: bad.outputTrim = -1; break;
            case 5: bad.schema = 0; break;
        }
        CHECK(current.load(bad) != Status::Ok);
        CHECK(current.loaded().stages[0].type == Type::Gain && current.loaded().outputTrim == .8f);
        for (size_t i = 0; i < 128; ++i) CHECK(current.process(signal(i)) == unchanged.process(signal(i)));
    }
    for (float rate : {44100.f, 0.f, NAN, INFINITY})
        CHECK(std::holds_alternative<prep::Error>(prep::prepare(document, rate)));
    auto processors = document.processors();
    auto order = document.serialOrder();
    processors[2].settings = data::Rat{.5f, .5f, .7f}; // ID40, already another RAT ID7.
    const auto tooMany = prep::detail::prepare({48000, .8f, 3, processors, order}, 48000);
    CHECK(std::holds_alternative<prep::Error>(tooMany));
    const auto& error = std::get<prep::Error>(tooMany);
    CHECK(error.nativeStatus == Status::TooManyRats && error.code == prep::ErrorCode::CapacityExceeded);
    CHECK(error.instanceId == 7 && error.rule.find("one Rat") != std::string::npos);
    processors = document.processors(); std::get<data::Rat>(processors[1].settings).filter = NAN;
    CHECK(std::holds_alternative<prep::Error>(prep::detail::prepare({48000, .8f, 3, processors, order}, 48000)));
    for (size_t i = 0; i < 4096; ++i) CHECK(current.process(signal(i)) == unchanged.process(signal(i)));

    Bank bank(48000);
    const std::array<std::string, 4> saved{fixture("rat.json"), fixture("rat-composite.json"),
        fixture("rat-reversed.json"), fixture("empty.json")};
    for (size_t slot = 0; slot < saved.size(); ++slot) CHECK(bank.set(slot, native(decode(saved[slot]))) == Status::Ok);
    for (size_t slot : {0u, 1u, 2u, 3u, 0u}) {
        CHECK(bank.select(slot) == Status::Ok);
        CHECK(current.load(bank.get(bank.active())) == Status::Ok);
        Engine fresh(48000); CHECK(fresh.load(native(decode(saved[slot]))) == Status::Ok);
        for (size_t i = 0; i < 2048; ++i) CHECK(current.process(signal(i)) == fresh.process(signal(i)));
    }
}

static void ratesControlsAndSafety() {
    auto j = Json::parse(fixture("rat.json"));
    for (float rate : {32000.f, 48000.f, 96000.f})
        for (const std::string varying : {"distortion", "filter", "volume"}) for (float control : {0.f, 1.f}) {
        j["sampleRate"]["value"] = rate;
        dsp::RatControls expected{.35f, .5f, .7f};
        if (varying == "distortion") expected.distortion = control;
        if (varying == "filter") expected.filter = control;
        if (varying == "volume") expected.volume = control;
        auto& parameters = j["processors"][0]["parameters"];
        parameters["distortion"]["value"] = expected.distortion;
        parameters["filter"]["value"] = expected.filter;
        parameters["volume"]["value"] = expected.volume;
        const auto p = native(decode(j.dump()), rate);
        Engine e(rate), skip(rate); CHECK(e.load(p) == Status::Ok); CHECK(skip.load(p) == Status::Ok);
        dsp::Rat reference; CHECK(reference.configure(rate, expected));
        for (size_t i = 0; i < 1024; ++i) {
            const float x = signal(i);
            const float y = e.process(x);
            CHECK(y == skip.process(x));
            CHECK(y == reference.process(x) * .8f);
        }
        for (float bad : {NAN, INFINITY, -INFINITY}) CHECK(e.process(bad) == 0);
        for (size_t i = 0; i < 2048; ++i) CHECK(e.process(signal(i)) == skip.process(signal(i)));
        uint32_t bits = 1234567;
        watching = true;
        for (size_t i = 0; i < 8192; ++i) {
            bits = bits * 1664525u + 1013904223u;
            float x; std::memcpy(&x, &bits, sizeof(x)); bounded(e.process(x));
        }
        for (float x : {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0.f}) bounded(e.process(x));
        watching = false; CHECK(allocations == 0);
        e.setVolume(0); CHECK(e.process(.1f) == 0);
        e.setVolume(NAN); CHECK(e.process(.1f) == 0);
    }
    Patch rat; rat.count = 1; rat.stages[0].type = Type::Rat;
    CHECK(validate(rat, 8000) == Status::UnsupportedRatRate);
    rat.count = 2; rat.stages[1] = rat.stages[0]; rat.stages[1].enabled = false;
    CHECK(validate(rat, 48000) == Status::TooManyRats);
}
int main() {
    contract(); audioAndOrdering(); failuresAndPresets(); ratesControlsAndSafety();
    std::cout << "RAT patch checks " << testChecks << "; process allocations " << allocations << '\n';
}
