#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

#include <d2df/core/log.hpp>
#include <d2df/tools/map_import_pipeline.hpp>

namespace {

void print_usage() {
    std::cout << "Usage: d2df-map-import [options]\n"
              << "  --content PATH   default: assets/content\n"
              << "  --dry-run\n";
}

d2df::tools::MapImportOptions parse_args(int argc, char* argv[]) {
    d2df::tools::MapImportOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--content" && i + 1 < argc) {
            opts.content_root = argv[++i];
        } else if (arg == "--dry-run") {
            opts.dry_run = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            print_usage();
            std::exit(1);
        }
    }
    return opts;
}

} // namespace

int main(int argc, char* argv[]) {
    d2df::init_logging();
    const auto opts = parse_args(argc, argv);
    spdlog::info("d2df-map-import: {}", opts.content_root.string());

    std::vector<d2df::tools::MapImportIssue> issues;
    const auto stats = d2df::tools::run_map_import(opts, issues);
    return stats.failed > 0 ? 1 : 0;
}
