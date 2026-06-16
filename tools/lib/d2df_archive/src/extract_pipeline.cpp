#include <d2df/tools/extract_pipeline.hpp>

#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <span>
#include <sstream>

#include <spdlog/spdlog.h>

#include <d2df/tools/archive.hpp>
#include <d2df/tools/ffmpeg_util.hpp>
#include <d2df/tools/format_sniff.hpp>
#include <d2df/tools/json_util.hpp>
#include <d2df/tools/nested_resource.hpp>
#include <d2df/tools/slug.hpp>
#include <d2df/tools/text_encoding.hpp>

namespace d2df::tools {
namespace {

constexpr std::array<const char*, 7> kArchiveExtensions = {
    ".wad", ".WAD", ".dfz", ".DFZ", ".dfwad", ".zip", ".pk3",
};

bool is_archive_path(const std::filesystem::path& path) {
    if (!path.has_extension()) {
        return false;
    }
    const auto ext = path.extension().string();
    for (const auto* candidate : kArchiveExtensions) {
        if (ext == candidate) {
            return true;
        }
    }
    return false;
}

std::string kind_to_string(AssetKind kind) {
    switch (kind) {
    case AssetKind::Image:
        return "image";
    case AssetKind::Audio:
        return "audio";
    case AssetKind::Map:
        return "map";
    case AssetKind::FontConfig:
        return "font";
    default:
        return "binary";
    }
}

void write_bytes(const std::filesystem::path& path, std::span<const std::uint8_t> data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to write: " + path.string());
    }
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

std::filesystem::path archive_staging_root(const std::filesystem::path& output_root,
                                           const std::filesystem::path& archive_path,
                                           const std::filesystem::path& input_root) {
    std::error_code ec;
    const auto rel = std::filesystem::relative(archive_path, input_root, ec);
    if (!ec) {
        return output_root / rel.parent_path() / archive_path.filename();
    }
    return output_root / archive_path.filename();
}

void collect_archives(const std::filesystem::path& root, std::vector<std::filesystem::path>& out) {
    if (!std::filesystem::exists(root)) {
        spdlog::warn("Input path does not exist: {}", root.string());
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (is_archive_path(entry.path())) {
            out.push_back(entry.path());
        }
    }
}

std::optional<std::string> convert_with_ffmpeg(const ExtractOptions& options, AssetKind kind,
                                               const std::filesystem::path& src,
                                               const std::filesystem::path& dst) {
    std::vector<std::string> args;
    if (kind == AssetKind::Audio) {
        args = {"-c:a", "libvorbis", "-q:a", "6"};
    } else if (kind == AssetKind::Image) {
        args = {};
    } else {
        return "unsupported asset kind for ffmpeg";
    }

    const auto result = ffmpeg_convert(options.ffmpeg, src, dst, args);
    if (result.exit_code != 0) {
        spdlog::debug("ffmpeg failed ({}): {}", dst.string(), result.stderr_text);
        return result.stderr_text.empty() ? std::optional<std::string>{"ffmpeg failed"}
                                          : result.stderr_text;
    }
    return std::nullopt;
}

void process_archive(const ExtractOptions& options, const std::filesystem::path& archive_path,
                     ExtractStats& stats, std::vector<StagingRecord>& records,
                     std::vector<ExtractIssue>& issues, bool ffmpeg_ok, std::size_t archive_index,
                     std::size_t archive_total) {
    spdlog::info("[{}/{}] {}", archive_index, archive_total, archive_path.filename().string());
    ArchiveFile archive;
    try {
        archive = open_archive_file(archive_path.string());
    } catch (const std::exception& ex) {
        spdlog::error("  {}", ex.what());
        ++stats.errors;
        return;
    }

    ++stats.archives;
    const auto staging_root =
        archive_staging_root(options.output_root, archive_path, options.input_root);

    SlugGenerator slug_gen;

    for (const auto& entry : archive.entries) {
        if (entry.is_directory) {
            continue;
        }
        ++stats.entries;

        StagingRecord record;
        record.archive = archive_path.filename().string();
        record.virtual_path = entry.path;
        record.original_name = decode_legacy_text(entry.name);
        record.legacy_ref = entry_legacy_ref(archive_path.filename().string(), entry);
        record.convert_status = "none";

        try {
            auto bytes = extract_entry(archive, entry);
            const auto unwrapped = unwrap_nested_resource(bytes);
            if (unwrapped.was_nested) {
                bytes = std::move(unwrapped.bytes);
                ++stats.nested_unwrapped;
            }

            const auto format = sniff_format_with_name(bytes, record.original_name);
            record.kind = kind_to_string(format.kind);
            record.source_format = format.extension;
            record.size_bytes = bytes.size();

            if (format.kind == AssetKind::Binary) {
                ++stats.kept_binary;
            }

            const auto slug = slug_gen.make(record.original_name);
            record.slug = slug;

            auto rel_dir = staging_root / entry.path;
            auto raw_name = slug + "." + format.extension;
            auto raw_path = rel_dir / raw_name;
            record.disk_path =
                std::filesystem::relative(raw_path, options.output_root).generic_string();

            if (!options.dry_run) {
                std::filesystem::create_directories(rel_dir);
                write_bytes(raw_path, bytes);
            }

            std::filesystem::path final_path = raw_path;
            record.output_format = format.extension;

            const bool path_suggests_audio =
                entry.path.find("D2DMUS/") != std::string::npos ||
                entry.path.find("STDMUS/") != std::string::npos ||
                entry.path.find("MUSIC/") != std::string::npos ||
                entry.path.find("SOUNDS/") != std::string::npos ||
                entry.path.find("MSOUNDS/") != std::string::npos ||
                entry.path.find("CHATSND/") != std::string::npos ||
                entry.path.find("MUS_DOOM/") != std::string::npos ||
                entry.path.find("MUS_DOOM2/") != std::string::npos;

            const bool is_audio = format.kind == AssetKind::Audio ||
                                  (format.kind == AssetKind::Binary && path_suggests_audio);
            const bool want_audio = options.convert_audio && is_audio &&
                                    ffmpeg_can_convert_audio(format.extension);
            const bool want_image = options.convert_images && format.kind == AssetKind::Image &&
                                    format.extension != "png";

            if (options.convert_audio && is_audio && !ffmpeg_can_convert_audio(format.extension)) {
                record.convert_status = "skipped";
                ++stats.convert_skipped;
            }

            if ((want_audio || want_image) && !options.dry_run) {
                if (!ffmpeg_ok) {
                    if (options.verbose) {
                        issues.push_back({record.legacy_ref, "convert", "ffmpeg not found in PATH"});
                    }
                } else {
                    const auto out_ext = want_audio ? "ogg" : "png";
                    const AssetKind convert_kind = want_audio ? AssetKind::Audio : AssetKind::Image;
                    auto converted_path = rel_dir / (slug + "." + out_ext);
                    if (const auto convert_error =
                            convert_with_ffmpeg(options, convert_kind, raw_path, converted_path)) {
                        record.convert_status = "failed";
                        ++stats.convert_failed;
                        issues.push_back(
                            {record.legacy_ref, "convert", "ffmpeg failed: " + *convert_error});
                    } else {
                        std::filesystem::remove(raw_path);
                        final_path = converted_path;
                        record.output_format = out_ext;
                        record.disk_path = std::filesystem::relative(final_path, options.output_root)
                                               .generic_string();
                        record.convert_status = "ok";
                        ++stats.converted;
                    }
                }
            }

            records.push_back(std::move(record));
            ++stats.extracted;
        } catch (const std::exception& ex) {
            spdlog::error("  {}: {}", record.legacy_ref, ex.what());
            issues.push_back({record.legacy_ref, "extract", ex.what()});
            ++stats.errors;
        }
    }
}

void write_manifest(const ExtractOptions& options, const std::vector<StagingRecord>& records,
                    const ExtractStats& stats) {
    if (options.dry_run) {
        return;
    }

    const auto manifest_path = options.output_root / "manifest.json";
    std::filesystem::create_directories(options.output_root);

    std::ofstream out(manifest_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to write manifest: " + manifest_path.string());
    }

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"generator\": \"d2df-extract\",\n";
    out << "  \"input_root\": \"" << json_escape(options.input_root.generic_string()) << "\",\n";
    out << "  \"output_root\": \"" << json_escape(options.output_root.generic_string()) << "\",\n";
    out << "  \"stats\": { \"archives\":" << stats.archives << ", \"entries\":" << stats.entries
        << ", \"extracted\":" << stats.extracted << ", \"converted\":" << stats.converted
        << ", \"nested_unwrapped\":" << stats.nested_unwrapped
        << ", \"convert_failed\":" << stats.convert_failed
        << ", \"convert_skipped\":" << stats.convert_skipped
        << ", \"kept_binary\":" << stats.kept_binary << ", \"errors\":" << stats.errors
        << " },\n";
    out << "  \"entries\": [\n";

    for (std::size_t i = 0; i < records.size(); ++i) {
        const auto& rec = records[i];
        out << "    {";
        out << "\"legacy_ref\":\"" << json_escape(rec.legacy_ref) << "\",";
        out << "\"archive\":\"" << json_escape(rec.archive) << "\",";
        out << "\"virtual_path\":\"" << json_escape(rec.virtual_path) << "\",";
        out << "\"original_name\":\"" << json_escape(rec.original_name) << "\",";
        out << "\"slug\":\"" << json_escape(rec.slug) << "\",";
        out << "\"disk_path\":\"" << json_escape(rec.disk_path) << "\",";
        out << "\"kind\":\"" << json_escape(rec.kind) << "\",";
        out << "\"source_format\":\"" << json_escape(rec.source_format) << "\",";
        out << "\"output_format\":\"" << json_escape(rec.output_format) << "\",";
        out << "\"convert_status\":\"" << json_escape(rec.convert_status) << "\",";
        out << "\"size_bytes\":" << rec.size_bytes;
        out << "}";
        if (i + 1 < records.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n}\n";
    spdlog::info("Wrote {} ({} entries)", manifest_path.string(), records.size());
}

void write_extract_report(const ExtractOptions& options, const ExtractStats& stats,
                          const std::vector<ExtractIssue>& issues) {
    if (options.dry_run) {
        return;
    }

    const auto report_path = options.output_root / "extract_report.json";
    std::ofstream out(report_path, std::ios::binary);
    if (!out) {
        spdlog::warn("Failed to write extract report: {}", report_path.string());
        return;
    }

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"generator\": \"d2df-extract\",\n";
    out << "  \"stats\": { \"archives\":" << stats.archives << ", \"entries\":" << stats.entries
        << ", \"extracted\":" << stats.extracted << ", \"converted\":" << stats.converted
        << ", \"nested_unwrapped\":" << stats.nested_unwrapped
        << ", \"convert_failed\":" << stats.convert_failed
        << ", \"convert_skipped\":" << stats.convert_skipped
        << ", \"kept_binary\":" << stats.kept_binary << ", \"errors\":" << stats.errors
        << " },\n";
    out << "  \"issues\": [\n";

    for (std::size_t i = 0; i < issues.size(); ++i) {
        const auto& issue = issues[i];
        out << "    {\"legacy_ref\":\"" << json_escape(issue.legacy_ref) << "\",";
        out << "\"stage\":\"" << json_escape(issue.stage) << "\",";
        out << "\"message\":\"" << json_escape(issue.message) << "\"}";
        if (i + 1 < issues.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n}\n";
    spdlog::info("Wrote {} ({} issues)", report_path.string(), issues.size());
}

} // namespace

void write_extract_summary(const ExtractOptions& options, const ExtractStats& stats,
                           const std::vector<ExtractIssue>& issues) {
    if (options.dry_run) {
        return;
    }

    std::ostringstream text;
    text << "========================================\n";
    text << "  d2df-extract summary\n";
    text << "========================================\n";
    text << "Input:     " << options.input_root.generic_string() << '\n';
    text << "Output:    " << options.output_root.generic_string() << '\n';
    text << '\n';
    text << "Archives:          " << stats.archives << '\n';
    text << "Entries:           " << stats.entries << '\n';
    text << "Extracted:         " << stats.extracted << '\n';
    text << "Converted (OGG/PNG):" << stats.converted << '\n';
    text << "Nested WAD unwrap: " << stats.nested_unwrapped << '\n';
    text << "Convert skipped:   " << stats.convert_skipped << " (MIDI etc., kept as-is)\n";
    text << "Unknown (.bin):    " << stats.kept_binary << '\n';
    text << "Convert failed:    " << stats.convert_failed << '\n';
    text << "Extract errors:    " << stats.errors << '\n';

    if (!issues.empty()) {
        text << "\n--- Problems (" << issues.size() << ") ---\n";
        const std::size_t max_lines = 50;
        for (std::size_t i = 0; i < issues.size() && i < max_lines; ++i) {
            const auto& issue = issues[i];
            text << '[' << issue.stage << "] " << issue.legacy_ref << "\n    " << issue.message
                 << '\n';
        }
        if (issues.size() > max_lines) {
            text << "... and " << (issues.size() - max_lines)
                 << " more in extract_report.json\n";
        }
    } else {
        text << "\nNo problems.\n";
    }

    text << "\nFiles:\n";
    text << "  manifest.json\n";
    text << "  extract_report.json (full issue list)\n";
    text << "  extract_summary.txt (this file)\n";
    text << "========================================\n";

    const auto summary = text.str();
    std::cout << summary;

    const auto summary_path = options.output_root / "extract_summary.txt";
    std::ofstream out(summary_path, std::ios::binary);
    if (out) {
        out << summary;
    }
}

ExtractStats run_extract(const ExtractOptions& options, std::vector<StagingRecord>& records,
                         std::vector<ExtractIssue>& issues) {
    ExtractStats stats;
    std::vector<std::filesystem::path> archives;

    if (std::filesystem::is_regular_file(options.input_root)) {
        archives.push_back(options.input_root);
    } else {
        collect_archives(options.input_root, archives);
    }

    std::sort(archives.begin(), archives.end());
    spdlog::info("Found {} archive(s)", archives.size());

    const bool wants_convert = options.convert_audio || options.convert_images;
    const bool ffmpeg_ok = wants_convert && ffmpeg_available(options.ffmpeg);
    if (wants_convert && !ffmpeg_ok) {
        spdlog::warn("ffmpeg not available at '{}' — conversion disabled", options.ffmpeg.string());
    } else if (ffmpeg_ok) {
        spdlog::info("ffmpeg OK");
    }

    const auto archive_total = archives.size();
    for (std::size_t i = 0; i < archives.size(); ++i) {
        process_archive(options, archives[i], stats, records, issues, ffmpeg_ok, i + 1,
                        archive_total);
    }

    write_manifest(options, records, stats);
    write_extract_report(options, stats, issues);
    write_extract_summary(options, stats, issues);
    return stats;
}

} // namespace d2df::tools
