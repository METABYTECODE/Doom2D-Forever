#include <d2df/map/map_catalog.hpp>

#include <algorithm>

namespace d2df::map {

std::vector<std::filesystem::path> list_map_json_files(
    const std::filesystem::path& content_root) {
    std::vector<std::filesystem::path> maps;
    const auto maps_root = content_root / "maps";
    if (!std::filesystem::exists(maps_root)) {
        return maps;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(maps_root)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        if (entry.path().filename() == "import_report.json") {
            continue;
        }
        maps.push_back(entry.path());
    }

    std::sort(maps.begin(), maps.end());
    return maps;
}

} // namespace d2df::map
