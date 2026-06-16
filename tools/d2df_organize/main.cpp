#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

#include <d2df/core/log.hpp>
#include <d2df/tools/organize_pipeline.hpp>

namespace {

void print_usage() {
    std::cout << "Usage: d2df-organize [options]\n"
              << "  --staging PATH     default: assets/staging\n"
              << "  --output PATH      default: assets/content\n"
              << "  --overrides PATH   default: assets/organize_overrides.json\n"
              << "  --dry-run\n"
              << "  -v, --verbose\n";
}

d2df::tools::OrganizeOptions parse_args(int argc, char* argv[]) {
    d2df::tools::OrganizeOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--staging" && i + 1 < argc) {
            opts.staging_root = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            opts.output_root = argv[++i];
        } else if (arg == "--overrides" && i + 1 < argc) {
            opts.overrides_path = argv[++i];
        } else if (arg == "--dry-run") {
            opts.dry_run = true;
        } else if (arg == "-v" || arg == "--verbose") {
            spdlog::set_level(spdlog::level::debug);
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
    spdlog::info("d2df-organize: {} -> {}", opts.staging_root.string(), opts.output_root.string());

    std::vector<d2df::tools::ContentAsset> assets;
    const auto stats = d2df::tools::run_organize(opts, assets);

    spdlog::info(
        "Done: {} unique / {} input, {} organized, {} unclassified, {} duplicates skipped, {} errors",
        stats.unique_entries, stats.input_entries, stats.organized, stats.unclassified,
        stats.duplicates_skipped, stats.errors);

    return stats.errors > 0 ? 1 : 0;
}
