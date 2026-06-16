#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace d2df::tools {

struct FfmpegResult {
    int exit_code = -1;
    std::string stderr_text;
};

[[nodiscard]] bool ffmpeg_available(const std::filesystem::path& ffmpeg = "ffmpeg");

[[nodiscard]] FfmpegResult ffmpeg_convert(const std::filesystem::path& ffmpeg,
                                          const std::filesystem::path& input,
                                          const std::filesystem::path& output,
                                          const std::vector<std::string>& extra_args = {});

} // namespace d2df::tools
