#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>

namespace d2df::tools {

class SlugGenerator {
public:
    [[nodiscard]] std::string make(std::string_view utf8_text);

private:
    std::unordered_set<std::string> used_;
};

[[nodiscard]] std::string transliterate_utf8(std::string_view utf8_text);

[[nodiscard]] std::string sanitize_ascii_slug(std::string_view ascii);

} // namespace d2df::tools
