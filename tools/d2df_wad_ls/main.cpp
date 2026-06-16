#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

#include <d2df/tools/archive.hpp>

namespace {

void print_usage() {
    std::cout << "Usage: d2df-wad-ls <archive> [--path PREFIX]\n"
              << "       d2df-wad-ls --dir <folder>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::warn);

    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string path_filter;
    bool dir_mode = false;
    std::string target;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--dir" && i + 1 < argc) {
            dir_mode = true;
            target = argv[++i];
        } else if (arg == "--path" && i + 1 < argc) {
            path_filter = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else if (target.empty()) {
            target = arg;
        }
    }

    if (target.empty()) {
        print_usage();
        return 1;
    }

    try {
        if (dir_mode) {
            // Caller can pass dir; for now require explicit archives via d2df-extract.
            std::cerr << "Use d2df-extract for directory batch processing.\n";
            return 1;
        }

        const auto archive = d2df::tools::open_archive_file(target);
        std::cout << archive.source_path << " [" << (archive.type == d2df::tools::ArchiveType::Dfwad
                                                           ? "DFWAD"
                                                           : "ZIP")
                  << "] entries=" << archive.entries.size() << "\n";

        for (const auto& entry : archive.entries) {
            if (!path_filter.empty() && entry.path.find(path_filter) == std::string::npos) {
                continue;
            }
            if (entry.is_directory) {
                std::cout << "  [DIR]  " << entry.path << entry.name << "\n";
                continue;
            }
            std::cout << "  " << entry.path << entry.name << "  off=" << entry.offset
                      << " pack=" << entry.packed_size << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
