#include "parable_patch.h"
#include "patch_json.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <iterator>

namespace parable::patch {
namespace {
using detail::Json;
struct Descriptor { const char* type; const char* parameter; const char* unit; };
constexpr Descriptor types[] = {
    {"Bypass", "", ""}, {"Gain", "gain", "linear"}, {"OnePoleLowpass", "cutoff", "Hz"}, {"Rat", "", ""}, {"GeminiCmos", "", ""}
};
[[noreturn]] void fail(ErrorCode code, const std::string& field, const std::string& rule) {
    throw Error{code, field, rule};
}
std::string field(const std::string& parent, std::string_view key) {
    std::string path = parent + "/";
    for (char c : key) path += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
    return path;
}
void keys(const Json& value, const std::string& path, std::initializer_list<const char*> expected) {
    if (!value.is_object()) fail(ErrorCode::WrongType, path, "object required");
    for (const auto& key : expected)
        if (!value.contains(key)) fail(ErrorCode::MissingField, field(path, key), "required field missing");
    for (const auto& item : value.items()) {
        if (std::none_of(expected.begin(), expected.end(), [&](const char* key) { return item.key() == key; }))
            fail(ErrorCode::UnknownField, field(path, item.key()), "unknown field");
    }
}
void literal(const Json& value, const std::string& path, std::string_view expected, ErrorCode error) {
    if (!value.is_string()) fail(ErrorCode::WrongType, path, "string required");
    if (value.get_ref<const std::string&>() != expected) fail(error, path, "expected " + std::string(expected));
}
uint32_t integer(const Json& value, const std::string& path) {
    if (!value.is_number_integer()) fail(ErrorCode::WrongType, path, "unsigned integer token required");
    if ((value.is_number_unsigned() && value.get<uint64_t>() > UINT32_MAX)
        || (!value.is_number_unsigned() && (value.get<int64_t>() < 0 || value.get<int64_t>() > UINT32_MAX)))
        fail(ErrorCode::OutOfRange, path, "integer outside uint32 range");
    return value.get<uint32_t>();
}
float number(const detail::Document& doc, const Json& value, const std::string& path) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::digits == 24,
                  "Patch format requires IEEE-754 binary32");
    if (!value.is_number()) fail(ErrorCode::WrongType, path, "number required");
    const auto token = doc.decimalTokens.find(path);
    const std::string text = token == doc.decimalTokens.end() ? value.dump() : token->second;
    float result = 0.f;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result, std::chars_format::general);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || !std::isfinite(result))
        fail(ErrorCode::InvalidNumber, path, "number must convert to finite binary32 without overflow/underflow");
    return result == 0.f ? 0.f : result; // Normalize negative zero.
}
float quantity(const detail::Document& doc, const Json& value, const std::string& path,
               std::string_view unit, float low, float high, bool exclusiveHigh = false) {
    keys(value, path, {"unit", "value"});
    literal(value["unit"], field(path, "unit"), unit, ErrorCode::InvalidUnit);
    const float result = number(doc, value["value"], field(path, "value"));
    if (result < low || (exclusiveHigh ? result >= high : result > high))
        fail(ErrorCode::OutOfRange, field(path, "value"), "binary32 value must be >= "
             + std::to_string(low) + (exclusiveHigh ? " and < " : " and <= ") + std::to_string(high));
    return result;
}
struct Draft {
    float rate, trim;
    size_t count;
    std::array<Processor, kMaxProcessors> processors{};
    std::array<uint32_t, kMaxProcessors> order{};
};
Draft validate(const detail::Document& doc) {
    const auto& root = doc.root;
    keys(root, "$", {"format", "version", "sampleRate", "outputTrim", "processors", "routing"});
    literal(root["format"], "$/format", kFormat, ErrorCode::UnsupportedFormat);
    if (integer(root["version"], "$/version") != kVersion)
        fail(ErrorCode::UnsupportedVersion, "$/version", "only parable.patch version 1 is supported");
    Draft draft;
    draft.rate = quantity(doc, root["sampleRate"], "$/sampleRate", "Hz", 8000.f, 192000.f);
    draft.trim = quantity(doc, root["outputTrim"], "$/outputTrim", "linear", 0.f, 1.f);
    const auto& processors = root["processors"];
    if (!processors.is_array()) fail(ErrorCode::WrongType, "$/processors", "array required");
    draft.count = processors.size();
    if (draft.count > kMaxProcessors) fail(ErrorCode::LimitExceeded, "$/processors", "at most 8 processors");
    size_t rats = 0,cmos = 0;
    for (size_t i = 0; i < draft.count; ++i) {
        const auto& source = processors[i];
        const auto path = "$/processors/" + std::to_string(i);
        keys(source, path, {"id", "type", "revision", "enabled", "parameters"});
        auto& p = draft.processors[i];
        p.id = integer(source["id"], path + "/id");
        if (p.id == 0) fail(ErrorCode::OutOfRange, path + "/id", "instance ID must be nonzero");
        for (size_t j = 0; j < i; ++j)
            if (draft.processors[j].id == p.id) fail(ErrorCode::DuplicateId, path + "/id", "duplicate instance ID");
        if (integer(source["revision"], path + "/revision") != 1)
            fail(ErrorCode::UnsupportedVersion, path + "/revision", "only processor revision 1 is supported");
        if (!source["enabled"].is_boolean()) fail(ErrorCode::WrongType, path + "/enabled", "boolean required");
        p.enabled = source["enabled"].get<bool>();
        if (!source["type"].is_string()) fail(ErrorCode::WrongType, path + "/type", "string required");
        size_t type = 0;
        while (type < std::size(types) && source["type"] != types[type].type) ++type;
        if (type == std::size(types)) fail(ErrorCode::UnsupportedProcessor, path + "/type", "unsupported processor type");
        const auto& parameters = source["parameters"];
        const auto paramPath = path + "/parameters";
        if (type == 0) {
            keys(parameters, paramPath, {});
            p.settings = Bypass{};
        } else if (type == 3) {
            if (++rats > 1)
                fail(ErrorCode::LimitExceeded, path + "/type", "at most one Rat per patch, including disabled instances");
            if(cmos)fail(ErrorCode::LimitExceeded,path+"/type","one shared nonlinear slot: Rat OR GeminiCmos, including disabled instances");
            if (draft.rate < 32000.f || draft.rate > 96000.f)
                fail(ErrorCode::OutOfRange, "$/sampleRate/value", "Rat requires [32000,96000] Hz");
            keys(parameters, paramPath, {"distortion", "filter", "volume"});
            p.settings = Rat{
                quantity(doc, parameters["distortion"], paramPath + "/distortion", "normalized", 0.f, 1.f),
                quantity(doc, parameters["filter"], paramPath + "/filter", "normalized", 0.f, 1.f),
                quantity(doc, parameters["volume"], paramPath + "/volume", "normalized", 0.f, 1.f)};
        } else if(type==4) {
            if(++cmos+rats>1)fail(ErrorCode::LimitExceeded,path+"/type","one shared nonlinear slot: Rat OR GeminiCmos, including disabled instances");
            if(draft.rate<32000.f||draft.rate>96000.f)fail(ErrorCode::OutOfRange,"$/sampleRate/value","GeminiCmos requires [32000,96000] Hz");
            keys(parameters,paramPath,{"drive","volume","biasLeak"});
            const auto& leak=parameters["biasLeak"];
            keys(leak,paramPath+"/biasLeak",{"unit","value"});
            literal(leak["unit"],paramPath+"/biasLeak/unit","boolean",ErrorCode::InvalidUnit);
            if(!leak["value"].is_boolean())fail(ErrorCode::WrongType,paramPath+"/biasLeak/value","strict boolean required; no severity interpolation");
            p.settings=GeminiCmos{quantity(doc,parameters["drive"],paramPath+"/drive","normalized",0.f,1.f),
                quantity(doc,parameters["volume"],paramPath+"/volume","normalized",0.f,1.f),leak["value"].get<bool>()};
        } else {
            const auto& descriptor = types[type];
            keys(parameters, paramPath, {descriptor.parameter});
            const float v = quantity(doc, parameters[descriptor.parameter], field(paramPath, descriptor.parameter),
                                     descriptor.unit, type == 1 ? 0.f : 20.f,
                                     type == 1 ? 2.f : .45f * draft.rate, type == 2);
            if (type == 1) p.settings = Gain{v};
            else p.settings = OnePoleLowpass{v};
        }
    }

    const auto& routing = root["routing"];
    if (!routing.is_array()) fail(ErrorCode::WrongType, "$/routing", "array required");
    if (routing.size() > kMaxProcessors + 1) fail(ErrorCode::LimitExceeded, "$/routing", "at most 9 edges");
    if (routing.size() != draft.count + 1)
        fail(ErrorCode::InvalidRouting, "$/routing", "serial routing requires processor count + 1 edges");
    // Node indices: input=0, processor i=i+1, output=count+1. Wire IDs remain stable.
    const size_t output = draft.count + 1;
    std::array<int, kMaxProcessors + 2> next;
    next.fill(-1);
    std::array<unsigned, kMaxProcessors + 2> incoming{};
    auto endpoint = [&](const Json& value, const std::string& path, bool source) -> size_t {
        if (value.is_string()) {
            if (value == (source ? "input" : "output")) return source ? 0 : output;
            fail(ErrorCode::InvalidEndpoint, path, "only input source or output destination strings are supported");
        }
        const auto id = integer(value, path);
        for (size_t i = 0; i < draft.count; ++i) if (draft.processors[i].id == id) return i + 1;
        fail(ErrorCode::InvalidEndpoint, path, "dangling processor ID");
    };
    for (size_t i = 0; i < routing.size(); ++i) {
        const auto path = "$/routing/" + std::to_string(i);
        keys(routing[i], path, {"from", "to"});
        const auto from = endpoint(routing[i]["from"], path + "/from", true);
        const auto to = endpoint(routing[i]["to"], path + "/to", false);
        if (from == to) fail(ErrorCode::InvalidRouting, path, "self-feedback unsupported");
        if (next[from] != -1 || incoming[to] != 0)
            fail(ErrorCode::InvalidRouting, path, "duplicate edge, branch or merge unsupported");
        next[from] = static_cast<int>(to);
        ++incoming[to];
    }
    for (size_t i = 1; i < output; ++i)
        if (next[i] == -1 || incoming[i] != 1)
            fail(ErrorCode::InvalidRouting, "$/routing", "every processor needs one incoming and one outgoing edge");
    size_t node = 0;
    std::array<bool, kMaxProcessors + 2> visited{};
    for (size_t i = 0; i <= draft.count; ++i) {
        if (next[node] < 0) fail(ErrorCode::InvalidRouting, "$/routing", "disconnected serial path");
        node = static_cast<size_t>(next[node]);
        if (node == output) {
            if (i != draft.count) fail(ErrorCode::InvalidRouting, "$/routing", "disconnected processors/cycles outside serial path");
            break;
        }
        if (visited[node]) fail(ErrorCode::InvalidRouting, "$/routing", "cycle/feedback unsupported");
        visited[node] = true;
        if (i == draft.count) fail(ErrorCode::InvalidRouting, "$/routing", "serial path must end at output");
        draft.order[i] = draft.processors[node - 1].id;
    }
    std::sort(draft.processors.begin(), draft.processors.begin() + draft.count,
              [](const Processor& a, const Processor& b) { return a.id < b.id; });
    return draft;
}
std::string numeric(float value) {
    if (value == 0.f) return "0";
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::scientific, 8);
    return std::string(buffer, result.ptr);
}
std::string quantity(float value, std::string_view unit) {
    return "{\"unit\":\"" + std::string(unit) + "\",\"value\":" + numeric(value) + "}";
}
} // namespace

DecodeResult Codec::decode(std::string_view text) {
    try {
        const auto draft = validate(detail::parse(text));
        return ValidatedPatch{draft.rate, draft.trim, draft.count, draft.processors, draft.order};
    } catch (const Error& error) { return error; }
}

std::string Codec::encode(const ValidatedPatch& patch) {
    std::string text = "{\"format\":\"parable.patch\",\"outputTrim\":" + quantity(patch.trim_, "linear") + ",\"processors\":[";
    for (size_t i = 0; i < patch.count_; ++i) {
        if (i) text += ',';
        const auto& p = patch.processors_[i];
        const auto type = p.settings.index();
        text += "{\"enabled\":" + std::string(p.enabled ? "true" : "false") + ",\"id\":" + std::to_string(p.id) + ",\"parameters\":{";
        if (const auto* gain = std::get_if<Gain>(&p.settings)) text += "\"gain\":" + quantity(gain->linear, "linear");
        if (const auto* lowpass = std::get_if<OnePoleLowpass>(&p.settings)) text += "\"cutoff\":" + quantity(lowpass->cutoffHz, "Hz");
        if (const auto* rat = std::get_if<Rat>(&p.settings))
            text += "\"distortion\":" + quantity(rat->distortion, "normalized")
                + ",\"filter\":" + quantity(rat->filter, "normalized")
                + ",\"volume\":" + quantity(rat->volume, "normalized");
        if(const auto* cmos=std::get_if<GeminiCmos>(&p.settings))
            text+="\"biasLeak\":{\"unit\":\"boolean\",\"value\":"+std::string(cmos->biasLeak?"true":"false")+"},\"drive\":"
                +quantity(cmos->drive,"normalized")+",\"volume\":"+quantity(cmos->volume,"normalized");
        text += "},\"revision\":1,\"type\":\"" + std::string(types[type].type) + "\"}";
    }
    text += "],\"routing\":[";
    for (size_t i = 0; i <= patch.count_; ++i) {
        if (i) text += ',';
        text += "{\"from\":" + (i == 0 ? std::string("\"input\"") : std::to_string(patch.order_[i - 1]));
        text += ",\"to\":" + (i == patch.count_ ? std::string("\"output\"") : std::to_string(patch.order_[i])) + "}";
    }
    return text + "],\"sampleRate\":" + quantity(patch.rate_, "Hz") + ",\"version\":1}";
}
} // namespace parable::patch
