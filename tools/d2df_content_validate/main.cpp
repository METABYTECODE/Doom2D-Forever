#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

#include <d2df/core/log.hpp>
#include <d2df/tools/content_validate.hpp>

namespace {

void print_usage() {
    std::cout << "Usage: d2df-content-validate [options]\n"
              << "  --content PATH   default: assets/content\n";
}

d2df::tools::ContentValidateOptions parse_args(int argc, char* argv[]) {
    d2df::tools::ContentValidateOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--content" && i + 1 < argc) {
            opts.content_root = argv[++i];
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
    spdlog::info("d2df-content-validate: {}", opts.content_root.string());

    std::vector<d2df::tools::ContentValidateIssue> issues;
    const auto stats = d2df::tools::run_content_validate(opts, issues);
    return stats.missing_files > 0 || stats.broken_refs > 0 ? 1 : 0;
}
