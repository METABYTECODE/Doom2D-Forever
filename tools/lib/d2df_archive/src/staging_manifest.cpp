#include <d2df/tools/staging_manifest.hpp>

#include <fstream>
#include <stdexcept>

#include <algorithm>

#include <d2df/tools/json_util.hpp>

namespace d2df::tools {
namespace {

std::string lowercase_ascii(std::string_view text) {
    std::string out(text);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

std::optional<StagingRecord> parse_staging_line(std::string_view line) {
    if (line.find("\"legacy_ref\"") == std::string_view::npos) {
        return std::nullopt;
    }

    StagingRecord record;
    record.legacy_ref = json_extract_string(line, "legacy_ref").value_or("");
    record.archive = json_extract_string(line, "archive").value_or("");
    record.virtual_path = json_extract_string(line, "virtual_path").value_or("");
    record.original_name = json_extract_string(line, "original_name").value_or("");
    record.slug = json_extract_string(line, "slug").value_or("");
    record.disk_path = json_extract_string(line, "disk_path").value_or("");
    record.kind = json_extract_string(line, "kind").value_or("");
    record.source_format = json_extract_string(line, "source_format").value_or("");
    record.output_format = json_extract_string(line, "output_format").value_or("");
    record.size_bytes = json_extract_uint64(line, "size_bytes").value_or(0);

    if (record.legacy_ref.empty() || record.disk_path.empty()) {
        return std::nullopt;
    }
    return record;
}

bool prefer_staging_record(const StagingRecord& candidate, const StagingRecord& current) {
    const auto candidate_from_data = candidate.disk_path.rfind("data/", 0) == 0;
    const auto current_from_data = current.disk_path.rfind("data/", 0) == 0;
    if (candidate_from_data != current_from_data) {
        return candidate_from_data;
    }

    if (candidate.size_bytes != current.size_bytes) {
        return candidate.size_bytes > current.size_bytes;
    }

    return candidate.disk_path < current.disk_path;
}

} // namespace

std::vector<StagingRecord> load_staging_manifest(const std::filesystem::path& manifest_path) {
    std::ifstream in(manifest_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open staging manifest: " + manifest_path.string());
    }

    std::vector<StagingRecord> records;
    std::string line;
    while (std::getline(in, line)) {
        if (auto record = parse_staging_line(line)) {
            records.push_back(std::move(*record));
        }
    }
    return records;
}

std::vector<StagingRecord> dedupe_staging_records(const std::vector<StagingRecord>& records) {
    std::vector<StagingRecord> unique;
    unique.reserve(records.size());

    for (const auto& record : records) {
        const auto key = lowercase_ascii(record.legacy_ref);
        auto it = std::find_if(unique.begin(), unique.end(), [&](const StagingRecord& existing) {
            return lowercase_ascii(existing.legacy_ref) == key;
        });
        if (it == unique.end()) {
            unique.push_back(record);
            continue;
        }
        if (prefer_staging_record(record, *it)) {
            *it = record;
        }
    }
    return unique;
}

} // namespace d2df::tools
