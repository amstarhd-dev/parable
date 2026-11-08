#include "parable_patch.h"
#include "test_check.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <type_traits>

using namespace parable::patch;
using Json = nlohmann::json;
static std::string fixture(const std::string& name) {
    std::ifstream stream(std::string(PATCH_FIXTURES) + "/" + name, std::ios::binary);
    CHECK(stream.is_open());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
static ValidatedPatch decoded(std::string_view text) {
    const auto result = Codec::decode(text);
    if (const auto* error = std::get_if<Error>(&result))
        std::cerr << error->field << ": " << error->rule << '\n';
    CHECK(std::holds_alternative<ValidatedPatch>(result));
    return std::get<ValidatedPatch>(result);
}
static void rejected(std::string_view text, ErrorCode code, const std::string& field = {}) {
    const auto result = Codec::decode(text);
    CHECK(std::holds_alternative<Error>(result));
    const auto& error = std::get<Error>(result);
    if (error.code != code || (!field.empty() && error.field != field))
        std::cerr << "unexpected rejection " << static_cast<int>(error.code) << ' ' << error.field << ": " << error.rule << '\n';
    CHECK(error.code == code);
    CHECK(!error.field.empty() && !error.rule.empty());
    if (!field.empty()) CHECK(error.field == field);
}

static void goldenRoundTrips() {
    for (const std::string name : {"empty", "utilities", "disabled", "subnormal", "negative-zero", "cutoff-boundary"}) {
        const auto patch = decoded(fixture(name + ".json"));
        const auto canonical = fixture(name + ".canonical.json");
        CHECK(Codec::encode(patch) == canonical);
        const auto again = decoded(canonical);
        CHECK(again == patch);
        CHECK(Codec::encode(again) == canonical);
    }
    CHECK(!std::is_default_constructible<ValidatedPatch>::value);
    const auto p = decoded(fixture("utilities.json"));
    static_assert(std::is_const<std::remove_reference_t<decltype(p.processors()[0])>>::value,
                  "Validated processor data must be read-only");
    CHECK(p.count() == 4);
    CHECK(p.processors()[0].id == 10 && p.processors()[3].id == 40);
    CHECK(std::get<Gain>(p.processors()[1].settings).linear == .1f);
    CHECK(std::get<Gain>(p.processors()[3].settings).linear == 1.5f);
}

static void malformedFixtures() {
    struct Case { const char* name; ErrorCode code; const char* field; };
    const Case cases[] = {
        {"version", ErrorCode::UnsupportedVersion, "$/version"},
        {"format", ErrorCode::UnsupportedFormat, "$/format"},
        {"unknown-field", ErrorCode::UnknownField, "$/uiState"},
        {"missing-field", ErrorCode::MissingField, "$/outputTrim"},
        {"type", ErrorCode::UnsupportedProcessor, "$/processors/0/type"},
        {"revision", ErrorCode::UnsupportedVersion, "$/processors/0/revision"},
        {"duplicate-id", ErrorCode::DuplicateId, "$/processors/1/id"},
        {"enabled-type", ErrorCode::WrongType, "$/processors/0/enabled"},
        {"missing-parameter", ErrorCode::MissingField, "$/processors/0/parameters/gain"},
        {"extra-parameter", ErrorCode::UnknownField, "$/processors/0/parameters/extra"},
        {"unit", ErrorCode::InvalidUnit, "$/processors/0/parameters/gain/unit"},
        {"gain", ErrorCode::OutOfRange, "$/processors/0/parameters/gain/value"},
        {"cutoff-low", ErrorCode::OutOfRange, "$/processors/1/parameters/cutoff/value"},
        {"cutoff-high", ErrorCode::OutOfRange, "$/processors/1/parameters/cutoff/value"},
        {"trim", ErrorCode::OutOfRange, "$/outputTrim/value"},
        {"sample-rate", ErrorCode::OutOfRange, "$/sampleRate/value"},
        {"dangling", ErrorCode::InvalidEndpoint, "$/routing/0/to"},
        {"endpoint", ErrorCode::InvalidEndpoint, "$/routing/0/from"},
        {"branch", ErrorCode::InvalidRouting, "$/routing/1"},
        {"duplicate-edge", ErrorCode::InvalidRouting, "$/routing/1"},
        {"feedback", ErrorCode::InvalidRouting, "$/routing/1"},
        {"disconnected-cycle", ErrorCode::InvalidRouting, "$/routing"},
        {"duplicate-key", ErrorCode::DuplicateKey, "$/format"},
        {"escaped-duplicate", ErrorCode::DuplicateKey, "$/outputTrim/value"},
        {"overflow", ErrorCode::InvalidNumber, "$/processors/0/parameters/gain/value"},
        {"double-overflow", ErrorCode::InvalidNumber, "$/processors/0/parameters/gain/value"},
        {"underflow", ErrorCode::InvalidNumber, "$/processors/0/parameters/gain/value"},
        {"nan", ErrorCode::InvalidJson, "$/processors/0/parameters/gain/value"},
        {"inf", ErrorCode::InvalidJson, "$/processors/0/parameters/gain/value"},
        {"truncated", ErrorCode::InvalidJson, "$/format"}
    };
    const auto existing = decoded(fixture("utilities.json"));
    const auto before = Codec::encode(existing);
    for (const auto& test : cases) {
        rejected(fixture(std::string("invalid-") + test.name + ".json"), test.code, test.field);
        CHECK(Codec::encode(existing) == before); // No failed decode changes an existing snapshot.
    }
}

// Independent synthetic creators use the same public decode API. These contain
// no correspondence rules, chart inputs or astrology semantics.
static ValidatedPatch manualCreator() {
    Json board;
    board["format"] = "parable.patch";
    board["version"] = 1;
    board["sampleRate"] = {{"unit", "Hz"}, {"value", 48000}};
    board["outputTrim"] = {{"unit", "linear"}, {"value", .5}};
    board["processors"] = Json::array({{
        {"id", 7}, {"type", "Gain"}, {"revision", 1}, {"enabled", true},
        {"parameters", {{"gain", {{"unit", "linear"}, {"value", 1.5}}}}}
    }});
    board["routing"] = Json::array({{{"from", "input"}, {"to", 7}}, {{"from", 7}, {"to", "output"}}});
    return decoded(board.dump());
}
static ValidatedPatch chartLikeCreator() {
    const float syntheticValue = 3.f / 2.f;
    return decoded("{\"version\":1,\"sampleRate\":{\"value\":48000,\"unit\":\"Hz\"},"
        "\"format\":\"parable.patch\",\"outputTrim\":{\"unit\":\"linear\",\"value\":0.5},"
        "\"routing\":[{\"to\":\"output\",\"from\":7},{\"to\":7,\"from\":\"input\"}],"
        "\"processors\":[{\"enabled\":true,\"parameters\":{\"gain\":{\"value\":"
        + std::to_string(syntheticValue) + ",\"unit\":\"linear\"}},\"id\":7,\"type\":\"Gain\",\"revision\":1}]}");
}
static void creatorsIdentityAndSlots() {
    const auto manual = manualCreator(), chartLike = chartLikeCreator();
    CHECK(manual == chartLike);
    CHECK(manual == decoded(fixture("manual.json")));
    CHECK(chartLike == decoded(fixture("chart-like.json")));
    CHECK(Codec::encode(manual) == fixture("creators.canonical.json"));
    auto doc = Json::parse(fixture("utilities.json"));
    const auto original = decoded(doc.dump());
    std::reverse(doc["processors"].begin(), doc["processors"].end());
    std::reverse(doc["routing"].begin(), doc["routing"].end());
    CHECK(decoded(doc.dump()) == original); // Declaration/edge order has no playback meaning.
    doc["routing"] = Json::array({{{"from", "input"}, {"to", 20}}, {{"from", 20}, {"to", 10}},
        {{"from", 10}, {"to", 30}}, {{"from", 30}, {"to", 40}}, {{"from", 40}, {"to", "output"}}});
    const auto reordered = decoded(doc.dump());
    CHECK(reordered.processors() == original.processors());
    CHECK(reordered.serialOrder() != original.serialOrder());
    CHECK(reordered.serialOrder()[0] == 20 && original.serialOrder()[0] == 40);
    CHECK(decoded(Codec::encode(reordered)) == reordered);

    std::array<std::string, 4> slots{Codec::encode(manual), Codec::encode(original),
        Codec::encode(decoded(fixture("disabled.json"))), Codec::encode(decoded(fixture("subnormal.json")))};
    const auto saved = slots;
    for (size_t i = 0; i < slots.size(); ++i) {
        CHECK(Codec::encode(decoded(slots[i])) == slots[i]);
        for (size_t j = i + 1; j < slots.size(); ++j) CHECK(slots[i] != slots[j]);
    }
    auto edit = Json::parse(slots[0]);
    edit["processors"][0]["parameters"]["gain"]["value"] = .25;
    slots[0] = Codec::encode(decoded(edit.dump()));
    CHECK(slots[0] != saved[0]);
    CHECK(manual == manualCreator());
    for (size_t i = 1; i < slots.size(); ++i) CHECK(slots[i] == saved[i]);
}

static void limitsAndTypes() {
    const auto base = Json::parse(fixture("manual.json"));
    auto checkValue = [&](const char* path, Json value, ErrorCode code) {
        auto doc = base;
        doc[Json::json_pointer(path)] = std::move(value);
        rejected(doc.dump(), code, "$" + std::string(path));
    };
    for (Json value : {Json(true), Json("1"), Json(nullptr), Json::array()})
        checkValue("/processors/0/parameters/gain/value", value, ErrorCode::WrongType);
    for (Json value : {Json(0), Json(-1), Json(uint64_t{4294967296})})
        checkValue("/processors/0/id", value, ErrorCode::OutOfRange);
    checkValue("/processors/0/id", 1.0, ErrorCode::WrongType);
    checkValue("/version", 1.0, ErrorCode::WrongType);
    checkValue("/processors/0/parameters/gain/value", -.01, ErrorCode::OutOfRange);
    checkValue("/sampleRate/value", 192001, ErrorCode::OutOfRange);
    checkValue("/sampleRate/unit", "kHz", ErrorCode::InvalidUnit);
    checkValue("/outputTrim/value", -.01, ErrorCode::OutOfRange);
    checkValue("/routing/0/to", "7", ErrorCode::InvalidEndpoint);
    checkValue("/routing", Json::array(), ErrorCode::InvalidRouting);
    checkValue("/processors", Json::object(), ErrorCode::WrongType);
    checkValue("/routing", Json::object(), ErrorCode::WrongType);
    rejected("[]", ErrorCode::WrongType, "$");
    rejected(std::string("{\"format\":\"") + char(0xff) + "\"}", ErrorCode::InvalidJson, "$/format");
    rejected(fixture("manual.json") + "true", ErrorCode::InvalidJson, "$");
    rejected(fixture("manual.json") + std::string("\0trailing", 9), ErrorCode::InvalidJson, "$");
    rejected(std::string(kMaxDocumentBytes + 1, ' '), ErrorCode::LimitExceeded, "$");
    rejected("\xef\xbb\xbf" + fixture("manual.json"), ErrorCode::InvalidJson, "$");
    rejected("{\"" + std::string(65, 'x') + "\":0}", ErrorCode::LimitExceeded);
    rejected("{\"format\":\"" + std::string(65, 'x') + "\"}", ErrorCode::LimitExceeded, "$/format");
    rejected("{\"x\":" + std::string(8, '[') + "0" + std::string(8, ']') + "}", ErrorCode::LimitExceeded);
    std::string many = "[0";
    for (size_t i = 1; i < 512; ++i) many += ",0";
    rejected(many + "]", ErrorCode::LimitExceeded, "$/511");
    auto doc = base;
    doc["sampleRate"]["value"] = 192000;
    CHECK(decoded(doc.dump()).sampleRateHz() == 192000.f);
    doc = Json::parse(fixture("utilities.json"));
    doc["routing"][2]["to"] = 40;
    rejected(doc.dump(), ErrorCode::InvalidRouting, "$/routing/2"); // Merge into an already-fed node.
    doc = base;
    doc["routing"][0]["port"] = "left";
    rejected(doc.dump(), ErrorCode::UnknownField, "$/routing/0/port");
    doc = base;
    doc["processors"][0].erase("enabled");
    rejected(doc.dump(), ErrorCode::MissingField, "$/processors/0/enabled");
    doc = base;
    doc["processors"] = Json::array();
    for (uint32_t i = 0; i < 9; ++i) {
        auto p = base["processors"][0]; p["id"] = i + 1;
        doc["processors"].push_back(p);
    }
    rejected(doc.dump(), ErrorCode::LimitExceeded, "$/processors");
    doc = base;
    doc["routing"] = Json::array();
    for (int i = 0; i < 10; ++i) doc["routing"].push_back({{"from", "input"}, {"to", "output"}});
    rejected(doc.dump(), ErrorCode::LimitExceeded, "$/routing");
    // Accept every supported count, including zero and the eight-instance limit.
    for (uint32_t count = 0; count <= kMaxProcessors; ++count) {
        doc = base;
        doc["processors"] = Json::array(); doc["routing"] = Json::array();
        for (uint32_t id = 1; id <= count; ++id) {
            auto p = base["processors"][0]; p["id"] = id; doc["processors"].push_back(p);
            doc["routing"].push_back({{"from", id == 1 ? Json("input") : Json(id - 1)}, {"to", id}});
        }
        doc["routing"].push_back({{"from", count ? Json(count) : Json("input")}, {"to", "output"}});
        CHECK(decoded(doc.dump()).count() == count);
    }
}

static void numericRoundTrips() {
    auto source = fixture("manual.json");
    const auto start = source.find("1.5");
    CHECK(start != std::string::npos);
    auto value = [&](const std::string& token) {
        auto doc = source; doc.replace(start, 3, token); return decoded(doc);
    };
    CHECK(std::get<Gain>(value("1.000000059604644775390625").processors()[0].settings).linear == 1.f);
    CHECK(std::get<Gain>(value("1.0000000596046447753906251").processors()[0].settings).linear == std::nextafter(1.f, 2.f));
    CHECK(std::get<Gain>(value("2.0000000000000001").processors()[0].settings).linear == 2.f);
    auto nonFinite = source; nonFinite.replace(start, 3, "-Infinity");
    rejected(nonFinite, ErrorCode::InvalidJson, "$/processors/0/parameters/gain/value");
    auto longNumber = source; longNumber.replace(start, 3, "1." + std::string(64, '0'));
    rejected(longNumber, ErrorCode::LimitExceeded, "$/processors/0/parameters/gain/value");
    auto doc = Json::parse(source);
    uint32_t seed = 0x50415232;
    for (int i = 0; i < 4096; ++i) {
        seed = 1664525u * seed + 1013904223u;
        const uint32_t bits = seed & 0x3f7fffffu; // Positive binary32 values below one, including subnormals.
        float number; std::memcpy(&number, &bits, sizeof(number));
        doc["processors"][0]["parameters"]["gain"]["value"] = number;
        doc["outputTrim"]["value"] = number;
        const auto p = decoded(doc.dump());
        CHECK(p.outputTrimLinear() == number);
        CHECK(std::get<Gain>(p.processors()[0].settings).linear == number);
        const auto text = Codec::encode(p);
        CHECK(decoded(text) == p);
        CHECK(Codec::encode(decoded(text)) == text);
    }
}
int main() {
    goldenRoundTrips();
    malformedFixtures();
    creatorsIdentityAndSlots();
    limitsAndTypes();
    numericRoundTrips();
    std::cout << "PAR002 patch contract passed: " << testChecks << " checks\n";
}
