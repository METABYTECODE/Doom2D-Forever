#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace d2df::tools {

struct ExtractOptions {
    std::filesystem::path input_root = "assets";
    std::filesystem::path output_root = "assets/staging";
    std::filesystem::path ffmpeg = "ffmpeg";
    bool convert_audio = true;
    bool convert_images = true;
    bool dry_run = false;
    bool verbose = false;
};

struct ExtractStats {
    std::size_t archives = 0;
    std::size_t entries = 0;
    std::size_t extracted = 0;
    std::size_t converted = 0;
    std::size_t nested_unwrapped = 0;
    std::size_t convert_failed = 0;
    std::size_t convert_skipped = 0;
    std::size_t kept_binary = 0;
    std::size_t skipped = 0;
    std::size_t errors = 0;
};

struct ExtractIssue {
    std::string legacy_ref;
    std::string stage;   // extract | convert | sniff
    std::string message;
};

struct StagingRecord {
    std::string legacy_ref;
    std::string archive;
    std::string virtual_path;
    std::string original_name;
    std::string disk_path;
    std::string slug;
    std::string kind;
    std::string source_format;
    std::string output_format;
    std::string convert_status; // ok | failed | skipped | none
    std::uint64_t size_bytes = 0;
};

[[nodiscard]] ExtractStats run_extract(const ExtractOptions& options,
                                       std::vector<StagingRecord>& records,
                                       std::vector<ExtractIssue>& issues);

void write_extract_summary(const ExtractOptions& options, const ExtractStats& stats,
                           const std::vector<ExtractIssue>& issues);

} // namespace d2df::tools
