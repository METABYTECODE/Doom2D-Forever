#include <d2df/tools/text_encoding.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <vector>

namespace d2df::tools {
namespace {

bool decode_utf8_codepoint(std::string_view input, std::size_t& index, std::uint32_t& codepoint) {
    if (index >= input.size()) {
        return false;
    }

    const unsigned char lead = static_cast<unsigned char>(input[index]);
    if (lead < 0x80) {
        codepoint = lead;
        ++index;
        return true;
    }

    std::size_t extra = 0;
    if ((lead & 0xE0) == 0xC0) {
        extra = 1;
        codepoint = lead & 0x1F;
    } else if ((lead & 0xF0) == 0xE0) {
        extra = 2;
        codepoint = lead & 0x0F;
    } else if ((lead & 0xF8) == 0xF0) {
        extra = 3;
        codepoint = lead & 0x07;
    } else {
        return false;
    }

    if (index + extra >= input.size()) {
        return false;
    }

    for (std::size_t i = 1; i <= extra; ++i) {
        const unsigned char c = static_cast<unsigned char>(input[index + i]);
        if ((c & 0xC0) != 0x80) {
            return false;
        }
        codepoint = (codepoint << 6) | (c & 0x3F);
    }

    index += extra + 1;
    return true;
}

} // namespace

std::string cp1251_to_utf8(std::string_view input) {
    if (input.empty()) {
        return {};
    }

#ifdef _WIN32
    const int wide_len =
        MultiByteToWideChar(1251, MB_ERR_INVALID_CHARS, input.data(),
                            static_cast<int>(input.size()), nullptr, 0);
    if (wide_len <= 0) {
        return std::string(input);
    }

    std::wstring wide(static_cast<std::size_t>(wide_len), L'\0');
    MultiByteToWideChar(1251, MB_ERR_INVALID_CHARS, input.data(),
                        static_cast<int>(input.size()), wide.data(), wide_len);

    const int utf8_len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_len,
                                             nullptr, 0, nullptr, nullptr);
    if (utf8_len <= 0) {
        return std::string(input);
    }

    std::string utf8(static_cast<std::size_t>(utf8_len), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_len, utf8.data(),
                        utf8_len, nullptr, nullptr);
    return utf8;
#else
    // Minimal fallback: Latin-1 passthrough for non-Windows CI.
    std::string out;
    out.reserve(input.size());
    for (unsigned char b : input) {
        if (b < 0x80) {
            out.push_back(static_cast<char>(b));
        } else {
            out.push_back('?');
        }
    }
    return out;
#endif
}

std::string decode_legacy_text(std::string_view bytes) {
    return cp1251_to_utf8(bytes);
}

std::optional<std::string> utf8_latin1_mojibake_to_utf8(std::string_view utf8) {
    if (utf8.empty()) {
        return std::string{};
    }

    std::string cp1251_bytes;
    cp1251_bytes.reserve(utf8.size());
    bool has_high = false;

    for (std::size_t i = 0; i < utf8.size();) {
        std::uint32_t codepoint = 0;
        if (!decode_utf8_codepoint(utf8, i, codepoint)) {
            return std::nullopt;
        }

        if (codepoint > 0xFF) {
            return std::nullopt;
        }

        if (codepoint >= 0x80) {
            has_high = true;
        }
        cp1251_bytes.push_back(static_cast<char>(codepoint));
    }

    if (!has_high) {
        return std::nullopt;
    }

    return cp1251_to_utf8(cp1251_bytes);
}

} // namespace d2df::tools
