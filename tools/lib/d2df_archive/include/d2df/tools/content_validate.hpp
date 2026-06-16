#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace d2df::tools {

struct ContentValidateOptions {
    std::filesystem::path content_root = "assets/content";
};

struct ContentValidateStats {
    std::size_t assets = 0;
    std::size_t maps_checked = 0;
    std::size_t missing_files = 0;
    std::size_t broken_refs = 0;
};

struct ContentValidateIssue {
    std::string asset_id;
    std::string field;
    std::string message;
};

[[nodiscard]] ContentValidateStats run_content_validate(const ContentValidateOptions& options,
                                                        std::vector<ContentValidateIssue>& issues);

} // namespace d2df::tools
