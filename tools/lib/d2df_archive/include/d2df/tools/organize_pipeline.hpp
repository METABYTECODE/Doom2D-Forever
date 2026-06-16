#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace d2df::tools {

struct OrganizeOptions {
    std::filesystem::path staging_root = "assets/staging";
    std::filesystem::path output_root = "assets/content";
    std::filesystem::path overrides_path = "assets/organize_overrides.json";
    bool dry_run = false;
};

struct OrganizeStats {
    std::size_t input_entries = 0;
    std::size_t unique_entries = 0;
    std::size_t organized = 0;
    std::size_t unclassified = 0;
    std::size_t duplicates_skipped = 0;
    std::size_t id_collisions_resolved = 0;
    std::size_t errors = 0;
};

struct ContentAsset {
    std::string id;
    std::string type;
    std::string path;
    std::vector<std::string> legacy_refs;
    std::string original_name;
    std::string slug;
    std::string archive;
    std::string virtual_path;
    std::string output_format;
    std::uint64_t size_bytes = 0;
};

[[nodiscard]] OrganizeStats run_organize(const OrganizeOptions& options,
                                         std::vector<ContentAsset>& assets);

} // namespace d2df::tools
