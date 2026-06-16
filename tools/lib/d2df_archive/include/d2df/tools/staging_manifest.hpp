#pragma once

#include <d2df/tools/extract_pipeline.hpp>

#include <filesystem>
#include <vector>

namespace d2df::tools {

[[nodiscard]] std::vector<StagingRecord> load_staging_manifest(
    const std::filesystem::path& manifest_path);

[[nodiscard]] std::vector<StagingRecord> dedupe_staging_records(
    const std::vector<StagingRecord>& records);

} // namespace d2df::tools
