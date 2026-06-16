#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

#include <d2df/core/log.hpp>
#include <d2df/tools/extract_pipeline.hpp>

namespace {

void print_usage() {
    std::cout << "Usage: d2df-extract [options]\n"
              << "  --input PATH       default: assets\n"
              << "  --output PATH      default: assets/staging\n"
              << "  --ffmpeg PATH      default: ffmpeg\n"
              << "  --no-convert-audio\n"
              << "  --no-convert-images\n"
              << "  --dry-run\n"
              << "  --verbose          debug log + all issues in report\n"
              << "  -v, --verbose\n";
}

d2df::tools::ExtractOptions parse_args(int argc, char* argv[]) {
    d2df::tools::ExtractOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            opts.input_root = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            opts.output_root = argv[++i];
        } else if (arg == "--ffmpeg" && i + 1 < argc) {
            opts.ffmpeg = argv[++i];
        } else if (arg == "--no-convert-audio") {
            opts.convert_audio = false;
        } else if (arg == "--no-convert-images") {
            opts.convert_images = false;
        } else if (arg == "--dry-run") {
            opts.dry_run = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
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
    spdlog::info("d2df-extract: {} -> {}", opts.input_root.string(), opts.output_root.string());

    std::vector<d2df::tools::StagingRecord> records;
    std::vector<d2df::tools::ExtractIssue> issues;
    const auto stats = d2df::tools::run_extract(opts, records, issues);

    return stats.errors > 0 ? 1 : 0;
}
