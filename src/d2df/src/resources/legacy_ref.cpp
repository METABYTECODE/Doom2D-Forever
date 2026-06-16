#include <d2df/resources/legacy_ref.hpp>

namespace d2df::resources {

std::string normalize_legacy_ref(std::string_view ref) {
    std::string out(ref);
    for (char& c : out) {
        if (c == '\\') {
            c = '/';
        }
    }

    const auto colon = out.find(':');
    if (colon != std::string::npos) {
        for (std::size_t i = 0; i < colon; ++i) {
            if (out[i] >= 'A' && out[i] <= 'Z') {
                out[i] = static_cast<char>(out[i] - 'A' + 'a');
            }
        }
    }
    return out;
}

std::string extract_archive_from_legacy_ref(std::string_view ref) {
    const auto colon = ref.find(':');
    if (colon == std::string_view::npos) {
        return {};
    }
    return std::string(ref.substr(0, colon));
}

std::string qualify_map_legacy_ref(std::string_view ref, std::string_view map_legacy_ref) {
    if (ref.empty()) {
        return {};
    }
    if (ref.front() != ':') {
        return std::string(ref);
    }

    const auto archive = extract_archive_from_legacy_ref(map_legacy_ref);
    if (archive.empty()) {
        return std::string(ref);
    }

    std::string qualified = archive;
    qualified.append(ref);
    return qualified;
}

bool is_builtin_texture_ref(std::string_view ref) {
    return ref == "_water_0" || ref == "_water_1" || ref == "_water_2";
}

} // namespace d2df::resources
