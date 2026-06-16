#include <d2df/tools/slug.hpp>

#include <cctype>
#include <optional>
#include <unordered_set>

namespace d2df::tools {
namespace {

bool is_cyrillic_char32(char32_t cp) {
    return (cp >= 0x0400 && cp <= 0x04FF) || (cp >= 0x0500 && cp <= 0x052F);
}

std::string translit_cyrillic(char32_t cp) {
    switch (cp) {
    case 0x0410:
    case 0x0430:
        return "a";
    case 0x0411:
    case 0x0431:
        return "b";
    case 0x0412:
    case 0x0432:
        return "v";
    case 0x0413:
    case 0x0433:
        return "g";
    case 0x0414:
    case 0x0434:
        return "d";
    case 0x0415:
    case 0x0435:
    case 0x0401:
    case 0x0451:
        return "e";
    case 0x0416:
    case 0x0436:
        return "zh";
    case 0x0417:
    case 0x0437:
        return "z";
    case 0x0418:
    case 0x0438:
        return "i";
    case 0x0419:
    case 0x0439:
        return "y";
    case 0x041A:
    case 0x043A:
        return "k";
    case 0x041B:
    case 0x043B:
        return "l";
    case 0x041C:
    case 0x043C:
        return "m";
    case 0x041D:
    case 0x043D:
        return "n";
    case 0x041E:
    case 0x043E:
        return "o";
    case 0x041F:
    case 0x043F:
        return "p";
    case 0x0420:
    case 0x0440:
        return "r";
    case 0x0421:
    case 0x0441:
        return "s";
    case 0x0422:
    case 0x0442:
        return "t";
    case 0x0423:
    case 0x0443:
        return "u";
    case 0x0424:
    case 0x0444:
        return "f";
    case 0x0425:
    case 0x0445:
        return "h";
    case 0x0426:
    case 0x0446:
        return "ts";
    case 0x0427:
    case 0x0447:
        return "ch";
    case 0x0428:
    case 0x0448:
        return "sh";
    case 0x0429:
    case 0x0449:
        return "sch";
    case 0x042A:
    case 0x044A:
        return "";
    case 0x042B:
    case 0x044B:
        return "y";
    case 0x042C:
    case 0x044C:
        return "";
    case 0x042D:
    case 0x044D:
        return "e";
    case 0x042E:
    case 0x044E:
        return "yu";
    case 0x042F:
    case 0x044F:
        return "ya";
    default:
        return "";
    }
}

std::optional<char32_t> utf8_decode(std::string_view s, std::size_t& i) {
    if (i >= s.size()) {
        return std::nullopt;
    }
    const unsigned char c0 = s[i++];
    if (c0 < 0x80) {
        return c0;
    }
    if ((c0 & 0xE0) == 0xC0 && i < s.size()) {
        const unsigned char c1 = s[i++];
        return ((c0 & 0x1F) << 6) | (c1 & 0x3F);
    }
    if ((c0 & 0xF0) == 0xE0 && i + 1 < s.size()) {
        const unsigned char c1 = s[i++];
        const unsigned char c2 = s[i++];
        return ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
    }
    if ((c0 & 0xF8) == 0xF0 && i + 2 < s.size()) {
        const unsigned char c1 = s[i++];
        const unsigned char c2 = s[i++];
        const unsigned char c3 = s[i++];
        return ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    return std::nullopt;
}

} // namespace

std::string transliterate_utf8(std::string_view utf8_text) {
    std::string out;
    for (std::size_t i = 0; i < utf8_text.size();) {
        const auto cp = utf8_decode(utf8_text, i);
        if (!cp) {
            continue;
        }
        if (*cp < 0x80 && (std::isalnum(static_cast<unsigned char>(*cp)) || *cp == '_' || *cp == '-')) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*cp))));
        } else if (is_cyrillic_char32(*cp)) {
            out += translit_cyrillic(*cp);
        } else if (*cp == ' ' || *cp == '-' || *cp == '_') {
            out.push_back('_');
        }
    }
    return out;
}

std::string sanitize_ascii_slug(std::string_view ascii) {
    std::string out;
    for (char c : ascii) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) {
            out.push_back(static_cast<char>(std::tolower(uc)));
        } else if (c == '_' || c == '-' || c == '.') {
            out.push_back(c == '.' ? '_' : c);
        } else if (c == ' ') {
            out.push_back('_');
        }
    }
    while (!out.empty() && out.front() == '_') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out;
}

std::string SlugGenerator::make(std::string_view utf8_text) {
    auto slug = transliterate_utf8(utf8_text);
    if (slug.empty()) {
        slug = sanitize_ascii_slug(utf8_text);
    }
    if (slug.empty()) {
        slug = "entry";
    }

    std::string candidate = slug;
    int suffix = 2;
    while (used_.contains(candidate)) {
        candidate = slug + "_" + std::to_string(suffix++);
    }
    used_.insert(candidate);
    return candidate;
}

} // namespace d2df::tools
