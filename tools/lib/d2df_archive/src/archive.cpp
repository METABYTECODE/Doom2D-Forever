#include <d2df/tools/archive.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <span>

#include <d2df/tools/binary_reader.hpp>
#include <d2df/tools/zlib_util.hpp>

namespace d2df::tools {
namespace {

std::string join_path(const std::string& dir, const std::string& name) {
    if (dir.empty()) {
        return name;
    }
    if (dir.back() == '/') {
        return dir + name;
    }
    return dir + "/" + name;
}

std::string normalize_dfwad_name(const char* raw, std::size_t max_len) {
    std::string out;
    out.reserve(max_len);
    for (std::size_t i = 0; i < max_len && raw[i] != '\0'; ++i) {
        char c = raw[i];
        if (c == '\\') {
            c = '/';
        } else if (c == '/') {
            c = '_';
        }
        out.push_back(c);
    }
    return out;
}

void parse_dfwad(ArchiveFile& archive) {
    BinaryReader reader{archive.data};
    if (reader.remaining() < 8) {
        throw std::runtime_error("DFWAD: file too small");
    }

    const auto magic = reader.read_bytes(5);
    if (std::string_view(reinterpret_cast<const char*>(magic.data()), magic.size()) != "DFWAD") {
        throw std::runtime_error("DFWAD: bad magic");
    }
    const auto version = reader.read_u8();
    if (version != 0x01) {
        throw std::runtime_error("DFWAD: unsupported version");
    }

    const auto count = reader.read_u16_le();
    std::string cur_path;

    for (std::uint16_t i = 0; i < count; ++i) {
        auto name_bytes = reader.read_bytes(16);
        const auto offset = reader.read_u32_le();
        const auto packed_size = reader.read_u32_le();

        const auto fname = normalize_dfwad_name(reinterpret_cast<const char*>(name_bytes.data()),
                                                name_bytes.size());

        if (offset == 0 && packed_size == 0) {
            cur_path = fname;
            if (!cur_path.empty() && cur_path.back() != '/') {
                cur_path.push_back('/');
            }
            ArchiveEntry dir_entry;
            dir_entry.path = cur_path;
            dir_entry.name = fname;
            dir_entry.is_directory = true;
            archive.entries.push_back(std::move(dir_entry));
            continue;
        }

        if (fname.empty()) {
            continue;
        }

        ArchiveEntry entry;
        entry.path = cur_path;
        entry.name = fname;
        entry.offset = offset;
        entry.packed_size = packed_size;
        entry.unpacked_size = -1;
        entry.method = CompressionMethod::RawDeflate;
        archive.entries.push_back(std::move(entry));
    }
}

bool read_zip_local_header(BinaryReader& reader, ArchiveEntry& entry) {
    const auto sig = reader.read_bytes(4);
    if (sig[0] != 'P' || sig[1] != 'K') {
        return false;
    }

    if (sig[2] == 0x05 && sig[3] == 0x06) {
        return false; // end of central directory
    }
    if (sig[2] == 0x07 && sig[3] == 0x08) {
        reader.skip(12); // data descriptor
        return true;
    }
    if (sig[2] != 0x03 || sig[3] != 0x04) {
        return false;
    }

    reader.skip(2); // version
    reader.skip(2); // flags
    const auto method = reader.read_u16_le();
    reader.skip(8); // time, crc
    const auto packed_size = reader.read_u32_le();
    const auto unpacked_size = reader.read_u32_le();
    const auto name_len = reader.read_u16_le();
    const auto extra_len = reader.read_u16_le();

    auto name = reader.read_bytes(name_len);
    reader.skip(extra_len);

    std::string fname(reinterpret_cast<const char*>(name.data()), name.size());
    for (auto& c : fname) {
        if (c == '\\') {
            c = '/';
        }
    }

    const auto slash = fname.find_last_of('/');
    entry.path = slash == std::string::npos ? "" : fname.substr(0, slash + 1);
    entry.name = slash == std::string::npos ? fname : fname.substr(slash + 1);
    entry.offset = reader.position();
    entry.packed_size = packed_size;
    entry.unpacked_size = static_cast<std::int64_t>(unpacked_size);
    entry.method = method == 0   ? CompressionMethod::Store
                   : method == 8 ? CompressionMethod::Deflate
                                 : CompressionMethod::Store;
    entry.is_directory = fname.ends_with('/');

    if (!entry.is_directory) {
        reader.skip(packed_size);
    }

    return true;
}

void parse_zip(ArchiveFile& archive) {
    BinaryReader reader{archive.data};
    ArchiveEntry entry;

    while (!reader.eof()) {
        const auto pos = reader.position();
        if (reader.remaining() < 4) {
            break;
        }

        const auto b0 = archive.data[pos];
        const auto b1 = archive.data[pos + 1];
        if (b0 == 'P' && b1 == 'K') {
            entry = ArchiveEntry{};
            if (!read_zip_local_header(reader, entry)) {
                break;
            }
            if (!entry.name.empty() || entry.is_directory) {
                archive.entries.push_back(entry);
            }
            continue;
        }
        reader.skip(1);
    }
}

} // namespace

ArchiveType detect_archive_type(std::span<const std::uint8_t> data) {
    if (data.size() >= 5) {
        if (std::memcmp(data.data(), "DFWAD", 5) == 0) {
            return ArchiveType::Dfwad;
        }
    }
    if (data.size() >= 4) {
        if (data[0] == 'P' && data[1] == 'K' && data[2] == 0x03 && data[3] == 0x04) {
            return ArchiveType::Zip;
        }
        if (data[0] == 'P' && data[1] == 'K' && data[2] == 0x05 && data[3] == 0x06) {
            return ArchiveType::Zip;
        }
    }
    return ArchiveType::Unknown;
}

ArchiveFile open_archive_bytes(std::vector<std::uint8_t> data, std::string source_path) {
    ArchiveFile archive;
    archive.source_path = std::move(source_path);
    archive.data = std::move(data);
    archive.type = detect_archive_type(archive.data);

    switch (archive.type) {
    case ArchiveType::Dfwad:
        parse_dfwad(archive);
        break;
    case ArchiveType::Zip:
        parse_zip(archive);
        break;
    default:
        throw std::runtime_error("Unsupported archive format: " + archive.source_path);
    }

    return archive;
}

ArchiveFile open_archive_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open archive: " + path);
    }
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(data.data()), size);
    if (!in) {
        throw std::runtime_error("Failed to read archive: " + path);
    }
    return open_archive_bytes(std::move(data), path);
}

std::vector<std::uint8_t> extract_entry(const ArchiveFile& archive, const ArchiveEntry& entry) {
    if (entry.is_directory) {
        return {};
    }
    if (entry.offset + entry.packed_size > archive.data.size()) {
        throw std::runtime_error("Entry data out of range: " + join_path(entry.path, entry.name));
    }

    const auto* packed = archive.data.data() + entry.offset;
    const std::span<const std::uint8_t> packed_span(packed, static_cast<std::size_t>(entry.packed_size));

    switch (entry.method) {
    case CompressionMethod::Store:
        return {packed, packed + entry.packed_size};
    case CompressionMethod::Deflate:
    case CompressionMethod::RawDeflate: {
        const auto hint = entry.unpacked_size > 0 ? static_cast<std::size_t>(entry.unpacked_size) : 0;
        return zlib_inflate_raw(packed_span, hint);
    }
    default:
        throw std::runtime_error("Unsupported compression method");
    }
}

std::string entry_legacy_ref(std::string_view archive_path, const ArchiveEntry& entry) {
    std::string ref(archive_path);
    ref.push_back(':');
    if (!entry.path.empty()) {
        ref += entry.path;
    }
    ref += entry.name;
    return ref;
}

} // namespace d2df::tools
