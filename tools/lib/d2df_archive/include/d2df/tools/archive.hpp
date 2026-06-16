#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace d2df::tools {

enum class ArchiveType { Dfwad, Zip, Unknown };

enum class CompressionMethod : std::uint8_t {
    Store = 0,
    Deflate = 8,
    RawDeflate = 255,
};

struct ArchiveEntry {
    std::string path;          // virtual path inside archive (POSIX slashes)
    std::string name;          // entry file name
    std::uint64_t offset = 0;
    std::uint64_t packed_size = 0;
    std::int64_t unpacked_size = -1; // -1 = unknown until decompressed
    CompressionMethod method = CompressionMethod::RawDeflate;
    bool is_directory = false;
};

struct ArchiveFile {
    std::string source_path;
    ArchiveType type = ArchiveType::Unknown;
    std::vector<std::uint8_t> data;
    std::vector<ArchiveEntry> entries;
};

[[nodiscard]] ArchiveType detect_archive_type(std::span<const std::uint8_t> data);

[[nodiscard]] ArchiveFile open_archive_file(const std::string& path);
[[nodiscard]] ArchiveFile open_archive_bytes(std::vector<std::uint8_t> data, std::string source_path);

[[nodiscard]] std::vector<std::uint8_t> extract_entry(const ArchiveFile& archive,
                                                      const ArchiveEntry& entry);

[[nodiscard]] std::string entry_legacy_ref(std::string_view archive_path, const ArchiveEntry& entry);

} // namespace d2df::tools
