#include "parable_prepare.h"
#include "prepare_internal.h"
#include "test_check.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>

using namespace parable;
namespace data = parable::patch;
namespace prep = parable::preparation;
using Json = nlohmann::json;

static std::string fixture(const std::string& name) {
    std::ifstream file(std::string(PATCH_FIXTURES) + "/" + name, std::ios::binary);
    CHECK(file.is_open());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
static data::ValidatedPatch decode(const std::string& text) {
    const auto result = data::Codec::decode(text);
    CHECK(std::holds_alternative<data::ValidatedPatch>(result));
    return std::get<data::ValidatedPatch>(result);
}
static Patch success(const data::ValidatedPatch& source, float rate) {
    const auto result = prep::prepare(source, rate);
    if (const auto* e = std::get_if<prep::Error>(&result)) std::cerr << e->field << ": " << e->rule << '\n';
    CHECK(std::holds_alternative<Patch>(result));
    return std::get<Patch>(result);
}
static void samePatch(const Patch& actual, const Patch& expected) {
    CHECK(actual.schema == expected.schema);
    CHECK(actual.count == expected.count);
    CHECK(actual.outputTrim == expected.outputTrim);
    for (size_t i = 0; i < kMaxStages; ++i) {
        CHECK(actual.stages[i].type == expected.stages[i].type);
        CHECK(actual.stages[i].parameter == expected.stages[i].parameter);
        CHECK(actual.stages[i].enabled == expected.stages[i].enabled);
    }
}
static void bounded(float sample) {
    CHECK(std::isfinite(sample));
    CHECK(sample >= -1.f && sample <= 1.f);
}
static void render(const Patch& actual, const Patch& expected, float rate) {
    Engine playback(rate), reference(rate);
    // These synchronous host Engines have no running callback: load happens while stopped.
    CHECK(playback.load(actual) == Status::Ok);
    CHECK(reference.load(expected) == Status::Ok);
    for (int i = 0; i < 4096; ++i) {
        const float input = i == 0 ? .125f : i % 257 == 0 ? -.0625f
            : i < 1024 ? static_cast<float>(.03 * std::sin(i * .137))
            : i < 2048 ? .04f : i < 3072 ? -.02f : 0.f;
        const float y = playback.process(input), want = reference.process(input);
        bounded(y); bounded(want);
        CHECK(std::fabs(y - want) <= 1e-6f);
    }
    for (float input : {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), NAN, INFINITY, -INFINITY, 0.f, -.0f}) {
        const float y = playback.process(input), want = reference.process(input);
        bounded(y); bounded(want);
        CHECK(y == want);
    }
    for (int i = 0; i < 4096; ++i) {
        const float y = playback.process(i % 2 ? 0.f : .01f), want = reference.process(i % 2 ? 0.f : .01f);
        bounded(y);
        CHECK(std::fabs(y - want) <= 1e-6f);
    }
}

enum class Kind { Empty, Utilities, Disabled, Subnormal, Zero, Cutoff, Creator };
static Patch expectedPatch(Kind kind) {
    // Independently hand-built from fixture specifications; never derive stage order
    // or expected settings from a decoded document or the adapter under test.
    Patch p;
    switch (kind) {
        case Kind::Empty: break;
        case Kind::Utilities:
            p.count = 4; p.outputTrim = .8f;
            p.stages[0] = {Type::Gain, 1.5f, true};          // ID 40
            p.stages[1] = {Type::OnePoleLowpass, 1000.f, true}; // ID 10
            p.stages[2] = {Type::Bypass, 1.f, true};       // ID 30
            p.stages[3] = {Type::Gain, .1f, true};         // ID 20
            break;
        case Kind::Disabled:
            p.count = 1; p.stages[0] = {Type::Bypass, 1.f, false}; break;
        case Kind::Subnormal:
            p.count = 1; p.outputTrim = std::numeric_limits<float>::denorm_min();
            p.stages[0] = {Type::Gain, std::numeric_limits<float>::denorm_min(), true}; break;
        case Kind::Zero:
            p.count = 1; p.outputTrim = 0.f; p.stages[0] = {Type::Gain, 0.f, true}; break;
        case Kind::Cutoff:
            p.count = 1; p.stages[0] = {Type::OnePoleLowpass, 3599.999755859375f, true}; break;
        case Kind::Creator:
            p.count = 1; p.outputTrim = .5f; p.stages[0] = {Type::Gain, 1.5f, true}; break;
    }
    return p;
}
static void acceptedFixtures() {
    struct Case { const char* name; Kind kind; float rate; };
    const Case cases[] = {
        {"empty.json", Kind::Empty, 48000}, {"empty.canonical.json", Kind::Empty, 48000},
        {"utilities.json", Kind::Utilities, 48000}, {"utilities.canonical.json", Kind::Utilities, 48000},
        {"disabled.json", Kind::Disabled, 48000}, {"disabled.canonical.json", Kind::Disabled, 48000},
        {"subnormal.json", Kind::Subnormal, 48000}, {"subnormal.canonical.json", Kind::Subnormal, 48000},
        {"negative-zero.json", Kind::Zero, 48000}, {"negative-zero.canonical.json", Kind::Zero, 48000},
        {"cutoff-boundary.json", Kind::Cutoff, 8000}, {"cutoff-boundary.canonical.json", Kind::Cutoff, 8000},
        {"manual.json", Kind::Creator, 48000}, {"chart-like.json", Kind::Creator, 48000},
        {"creators.canonical.json", Kind::Creator, 48000}
    };
    for (const auto& test : cases) {
        const auto document = decode(fixture(test.name)), before = document;
        CHECK(document.sampleRateHz() == test.rate);
        const auto native = success(document, test.rate);
        const auto expected = expectedPatch(test.kind);
        samePatch(native, expected);
        CHECK(validate(native, test.rate) == Status::Ok);
        render(native, expected, test.rate);
        CHECK(document == before);
    }
}

static void orderFlagsAndOwnership() {
    auto json = Json::parse(fixture("utilities.json"));
    json["routing"] = Json::array({{{"from", "input"}, {"to", 20}}, {{"from", 20}, {"to", 10}},
        {{"from", 10}, {"to", 30}}, {{"from", 30}, {"to", 40}}, {{"from", 40}, {"to", "output"}}});
    const auto source = decode(json.dump()), before = source;
    auto expected = expectedPatch(Kind::Utilities);
    std::swap(expected.stages[0], expected.stages[3]); // Same IDs/settings, different explicit signal order.
    auto native = success(source, 48000);
    samePatch(native, expected);
    render(native, expected, 48000);
    // Gain/filter small-signal audio can commute. Structural comparisons above
    // are the order proof; do not claim an audible difference from this render.
    for (auto& p : json["processors"]) if (p["id"] == 10 || p["id"] == 20) p["enabled"] = false;
    expected.stages[0].enabled = false; expected.stages[1].enabled = false;
    auto disabled = success(decode(json.dump()), 48000);
    samePatch(disabled, expected);
    render(disabled, expected, 48000);

    Engine loaded(48000);
    CHECK(loaded.load(native) == Status::Ok); // Still no running callback.
    native.stages[0].parameter = 0; native.outputTrim = 0;
    auto original = expectedPatch(Kind::Utilities);
    std::swap(original.stages[0], original.stages[3]);
    samePatch(loaded.loaded(), original); // Engine owns its own copy too.
    json["outputTrim"]["value"] = 0;
    CHECK(source == before);
    samePatch(success(source, 48000), original);
    CHECK(success(decode(fixture("empty.json")), 48000).count == 0);
    samePatch(loaded.loaded(), original); // A later prepare does not alter old results/engine.

    // Existing emergency saturation makes these two gains noncommute only on
    // overload. This is a numerical rail demonstration, not a musical audition.
    auto overload = Json::parse(fixture("manual.json"));
    overload["outputTrim"]["value"] = .01;
    auto gain = overload["processors"][0]; gain["id"] = 9; gain["parameters"]["gain"]["value"] = 2;
    auto attenuator = gain; attenuator["id"] = 3; attenuator["parameters"]["gain"]["value"] = .25;
    overload["processors"] = Json::array({gain, attenuator});
    overload["routing"] = Json::array({{{"from", "input"}, {"to", 9}}, {{"from", 9}, {"to", 3}}, {{"from", 3}, {"to", "output"}}});
    const auto first = decode(overload.dump());
    overload["routing"] = Json::array({{{"from", "input"}, {"to", 3}}, {{"from", 3}, {"to", 9}}, {{"from", 9}, {"to", "output"}}});
    const auto second = decode(overload.dump());
    CHECK(first.processors() == second.processors());
    Engine a(48000), b(48000);
    CHECK(a.load(success(first, 48000)) == Status::Ok);
    CHECK(b.load(success(second, 48000)) == Status::Ok);
    CHECK(std::fabs(a.process(.125f) - b.process(.125f)) < 1e-7f);
    CHECK(std::fabs(a.process(200.f) - .64f) < 1e-6f);
    CHECK(b.process(200.f) == 1.f);
}

static void failuresPreservePlayback() {
    const auto document = decode(fixture("utilities.json")), snapshot = document;
    const auto serialized = data::Codec::encode(document);
    Engine live(48000), reference(48000);
    const auto old = expectedPatch(Kind::Utilities);
    CHECK(live.load(old) == Status::Ok); CHECK(reference.load(old) == Status::Ok);
    CHECK(live.process(.4f) == reference.process(.4f));
    auto unaffected = [&] {
        CHECK(document == snapshot);
        CHECK(data::Codec::encode(document) == serialized);
        samePatch(live.loaded(), old);
        for (int i = 0; i < 32; ++i) CHECK(live.process(i % 2 ? -.04f : .02f) == reference.process(i % 2 ? -.04f : .02f));
    };
    auto reject = [&](const prep::Result& result, prep::ErrorCode code, const std::string& field,
                      uint32_t id = 0, Status status = Status::Ok) {
        CHECK(std::holds_alternative<prep::Error>(result));
        const auto& e = std::get<prep::Error>(result);
        CHECK(e.code == code); CHECK(e.field == field); CHECK(!e.rule.empty());
        CHECK(e.instanceId == id); CHECK(e.nativeStatus == status);
        unaffected();
    };
    for (float rate : {0.f, -1.f, 7999.f, 192001.f, NAN, INFINITY, -INFINITY})
        reject(prep::prepare(document, rate), prep::ErrorCode::InvalidTargetRate, "target.sampleRateHz", 0, Status::InvalidParameter);
    for (float rate : {44100.f, 8000.f, 192000.f, std::nextafter(48000.f, INFINITY)})
        reject(prep::prepare(document, rate), prep::ErrorCode::SampleRateMismatch, "target.sampleRateHz");

    // ValidatedPatch cannot publicly contain these states. Exercise the exact
    // private preparation core with owned copies, not const_cast or a public bypass.
    auto processors = document.processors();
    auto order = document.serialOrder();
    prep::detail::SourceView view{48000, .8f, document.count(), processors, order};
    view.count = kMaxStages + 1;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::CapacityExceeded, "document.count");
    view.count = std::numeric_limits<size_t>::max();
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::CapacityExceeded, "document.count");
    view.count = document.count();
    order[0] = 99;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::UnresolvedInstance, "document.serialOrder[0]", 99);
    order[0] = 0;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::UnresolvedInstance, "document.serialOrder[0]");
    order = document.serialOrder(); order[1] = order[0];
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::InvalidSource, "document.serialOrder[1]", 40);
    order = document.serialOrder(); processors[1].id = processors[0].id;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::InvalidSource, "document.processors[id=10].id", 10);
    processors = document.processors(); processors[3].settings = data::Gain{NAN};
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::NativeValidationFailed, "document.processors[id=40].gain.linear", 40, Status::InvalidParameter);
    processors[3].settings = data::Gain{3.f}; processors[3].enabled = false;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::NativeValidationFailed, "document.processors[id=40].gain.linear", 40, Status::InvalidParameter);
    processors = document.processors(); processors[0].settings = data::OnePoleLowpass{21600.f};
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::NativeValidationFailed, "document.processors[id=10].cutoffHz", 10, Status::InvalidParameter);
    processors = document.processors(); view.outputTrim = NAN;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::NativeValidationFailed, "document.outputTrimLinear", 0, Status::InvalidParameter);
    view.outputTrim = .8f; view.sampleRate = NAN;
    reject(prep::detail::prepare(view, 48000), prep::ErrorCode::InvalidSource, "document.sampleRateHz", 0, Status::InvalidParameter);

    auto unsupported = Json::parse(fixture("manual.json"));
    unsupported["processors"][0]["type"] = "Unsupported";
    const auto invalid = data::Codec::decode(unsupported.dump());
    CHECK(std::holds_alternative<data::Error>(invalid));
    CHECK(std::get<data::Error>(invalid).code == data::ErrorCode::UnsupportedProcessor);
    CHECK(std::get<data::Error>(invalid).field == "$/processors/0/type");
    unaffected(); // Unsupported types never reach preparation through the public API.
}

static void capacityAndRates() {
    auto doc = Json::parse(fixture("manual.json"));
    doc["processors"] = Json::array(); doc["routing"] = Json::array();
    auto gain = Json::parse(fixture("manual.json"))["processors"][0];
    Patch expected; expected.count = 8; expected.outputTrim = .5f;
    for (uint32_t i = 0; i < 8; ++i) {
        gain["id"] = 100 - i; gain["parameters"]["gain"]["value"] = .5;
        doc["processors"].push_back(gain);
        doc["routing"].push_back({{"from", i ? Json(101 - i) : Json("input")}, {"to", 100 - i}});
        expected.stages[i] = {Type::Gain, .5f, true};
    }
    doc["routing"].push_back({{"from", 93}, {"to", "output"}});
    for (float rate : {8000.f, 48000.f, 192000.f}) {
        doc["sampleRate"]["value"] = rate;
        const auto native = success(decode(doc.dump()), rate);
        samePatch(native, expected); render(native, expected, rate);
    }
}
int main() {
    acceptedFixtures();
    orderFlagsAndOwnership();
    failuresPreservePlayback();
    capacityAndRates();
    std::cout << "PAR004 host preparation passed: " << testChecks << " checks\n";
}
