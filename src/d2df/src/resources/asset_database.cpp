#include <d2df/resources/asset_database.hpp>

#include <stdexcept>

namespace d2df::resources {

void AssetDatabase::load(const std::filesystem::path& content_root) {
    catalog_.load(content_root);
}

const ContentAssetInfo* AssetDatabase::get(std::string_view asset_id) const {
    return catalog_.find_asset(asset_id);
}

std::optional<std::string> AssetDatabase::resolve_legacy(std::string_view legacy_ref) const {
    return catalog_.resolve_alias(legacy_ref);
}

std::filesystem::path AssetDatabase::path_for(std::string_view asset_id) const {
    return catalog_.asset_path(asset_id);
}

std::vector<std::uint8_t> AssetDatabase::read_bytes(std::string_view asset_id) const {
    const auto path = path_for(asset_id);
    if (path.empty()) {
        throw std::runtime_error(std::string("Unknown asset: ") + std::string(asset_id));
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open asset: " + path.string());
    }

    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    if (size < 0) {
        throw std::runtime_error("Failed to size asset: " + path.string());
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!in) {
        throw std::runtime_error("Failed to read asset: " + path.string());
    }
    return bytes;
}

} // namespace d2df::resources
