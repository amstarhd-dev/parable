#pragma once
// Private bounded JSON ingress. Preserve decimal tokens for direct binary32 conversion.
#include "parable_patch.h"
#include <map>
#include <nlohmann/json.hpp>

namespace parable::patch::detail {
using Json = nlohmann::json;
struct Document {
    Json root;
    std::map<std::string, std::string> decimalTokens;
};
Document parse(std::string_view text); // Throws Error on invalid input, caught by Codec.
} // namespace parable::patch::detail
