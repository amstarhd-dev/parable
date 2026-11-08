#include "patch_json.h"
#include <cmath>
#include <vector>

namespace parable::patch::detail {
namespace {
std::string escape(std::string_view key) {
    std::string result;
    for (char c : key) result += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
    return result;
}

class Reader final : public nlohmann::json_sax<Json> {
    struct Frame { Json* value; std::string path, key; };
    std::vector<Frame> stack_;
    size_t values_ = 0;
    std::string path() const {
        if (stack_.empty()) return "$";
        const auto& f = stack_.back();
        return f.path + "/" + (f.value->is_array() ? std::to_string(f.value->size()) : escape(f.key));
    }
    bool fail(ErrorCode code, std::string rule) {
        error = {code, path(), std::move(rule)};
        return false;
    }
    Json* append(Json value) {
        if (++values_ > kMaxValues) {
            fail(ErrorCode::LimitExceeded, "at most 512 JSON values");
            return nullptr;
        }
        if (stack_.empty()) { document.root = std::move(value); return &document.root; }
        auto& f = stack_.back();
        if (f.value->is_array()) {
            f.value->push_back(std::move(value));
            return &f.value->back();
        }
        return &((*f.value)[f.key] = std::move(value));
    }
    bool start(Json value) {
        if (stack_.size() >= kMaxDepth) return fail(ErrorCode::LimitExceeded, "at most 8 nested containers");
        const auto p = path();
        auto* child = append(std::move(value));
        if (!child) return false;
        stack_.push_back({child, p, {}});
        return true;
    }
public:
    Document document;
    Error error{ErrorCode::InvalidJson, "$", "invalid JSON"};
    bool null() override { return append(nullptr) != nullptr; }
    bool boolean(bool value) override { return append(value) != nullptr; }
    bool number_integer(number_integer_t value) override { return append(value) != nullptr; }
    bool number_unsigned(number_unsigned_t value) override { return append(value) != nullptr; }
    bool number_float(number_float_t value, const string_t& token) override {
        if (!std::isfinite(value)) return fail(ErrorCode::InvalidNumber, "finite JSON number required");
        if (token.size() > kMaxStringBytes) return fail(ErrorCode::LimitExceeded, "numeric token exceeds 64 bytes");
        document.decimalTokens.emplace(path(), token);
        return append(value) != nullptr;
    }
    bool string(string_t& value) override {
        if (value.size() > kMaxStringBytes) return fail(ErrorCode::LimitExceeded, "string exceeds 64 bytes");
        return append(value) != nullptr;
    }
    bool binary(binary_t&) override { return fail(ErrorCode::InvalidJson, "JSON binary values unsupported"); }
    bool start_object(std::size_t) override { return start(Json::object()); }
    bool key(string_t& key) override {
        auto& frame = stack_.back();
        frame.key = key;
        if (key.size() > kMaxStringBytes) return fail(ErrorCode::LimitExceeded, "key exceeds 64 bytes");
        if (frame.value->contains(key)) return fail(ErrorCode::DuplicateKey, "duplicate object key");
        return true;
    }
    bool end_object() override { stack_.pop_back(); return true; }
    bool start_array(std::size_t) override { return start(Json::array()); }
    bool end_array() override { stack_.pop_back(); return true; }
    bool parse_error(std::size_t byte, const std::string&, const nlohmann::detail::exception& ex) override {
        return fail(ex.id == 406 ? ErrorCode::InvalidNumber : ErrorCode::InvalidJson,
                    "JSON syntax/number error at byte " + std::to_string(byte) + ": " + ex.what());
    }
};
} // namespace

Document parse(std::string_view text) {
    if (text.size() > kMaxDocumentBytes)
        throw Error{ErrorCode::LimitExceeded, "$", "document exceeds 16384 bytes"};
    if (text.substr(0, 3) == "\xef\xbb\xbf")
        throw Error{ErrorCode::InvalidJson, "$", "UTF-8 BOM unsupported"};
    // The upstream lexer treats a raw NUL as end-of-input even in strict mode.
    // JSON permits escaped \u0000, never a literal NUL byte or hidden trailing data.
    const auto nul = text.find('\0');
    if (nul != std::string_view::npos)
        throw Error{ErrorCode::InvalidJson, "$", "raw NUL at byte " + std::to_string(nul)};
    Reader reader;
    if (!Json::sax_parse(text.begin(), text.end(), &reader)) throw reader.error;
    return std::move(reader.document);
}
} // namespace parable::patch::detail
