#include <d2df/tools/ffmpeg_util.hpp>

#include <cstdio>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace d2df::tools {

namespace {

std::string quote_shell_arg(const std::string& value) {
    if (value.find_first_of(" \t\"") == std::string::npos) {
        return value;
    }
    std::string out = "\"";
    for (char c : value) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

std::string path_for_shell(const std::filesystem::path& path) {
#ifdef _WIN32
    auto native = path;
    native.make_preferred();
    return native.string();
#else
    return path.string();
#endif
}

std::string trim_ffmpeg_message(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }

    std::string out;
    out.reserve(256);
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (line.find("ffmpeg version") != std::string::npos) {
            continue;
        }
        if (line.find("built with") != std::string::npos) {
            continue;
        }
        if (line.find("configuration:") != std::string::npos) {
            continue;
        }
        if (line.find("libav") != std::string::npos) {
            continue;
        }
        if (line.find("libsw") != std::string::npos) {
            continue;
        }
        if (line.find("Exiting with exit code") != std::string::npos) {
            continue;
        }
        if (!out.empty()) {
            out += " | ";
        }
        out += line;
        if (out.size() > 400) {
            break;
        }
    }
    return out.empty() ? "ffmpeg failed" : out;
}

#ifdef _WIN32

std::wstring quote_cmdline_arg(const std::wstring& value) {
    if (value.empty()) {
        return L"\"\"";
    }
    if (value.find_first_of(L" \t\"") == std::wstring::npos) {
        return value;
    }
    std::wstring out = L"\"";
    for (wchar_t c : value) {
        if (c == L'"') {
            out += L"\"\"";
        } else {
            out.push_back(c);
        }
    }
    out.push_back(L'"');
    return out;
}

FfmpegResult run_ffmpeg_windows(const std::wstring& command_line) {
    FfmpegResult result;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stderr_read = nullptr;
    HANDLE stderr_write = nullptr;
    HANDLE null_out = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0,
                                  nullptr);
    if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
        if (null_out != INVALID_HANDLE_VALUE) {
            CloseHandle(null_out);
        }
        result.stderr_text = "CreatePipe failed";
        return result;
    }
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = null_out != INVALID_HANDLE_VALUE ? null_out : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = stderr_write;

    PROCESS_INFORMATION pi{};
    std::wstring mutable_cmd = command_line;
    if (!CreateProcessW(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &si, &pi)) {
        CloseHandle(stderr_read);
        CloseHandle(stderr_write);
        if (null_out != INVALID_HANDLE_VALUE) {
            CloseHandle(null_out);
        }
        result.stderr_text = "CreateProcess failed";
        return result;
    }

    CloseHandle(stderr_write);
    CloseHandle(pi.hThread);
    if (null_out != INVALID_HANDLE_VALUE) {
        CloseHandle(null_out);
    }

    char buffer[512];
    DWORD bytes_read = 0;
    while (ReadFile(stderr_read, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        result.stderr_text += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = static_cast<DWORD>(-1);
    GetExitCodeProcess(pi.hProcess, &exit_code);
    result.exit_code = static_cast<int>(exit_code);

    CloseHandle(stderr_read);
    CloseHandle(pi.hProcess);
    result.stderr_text = trim_ffmpeg_message(std::move(result.stderr_text));
    return result;
}

#endif

} // namespace

bool ffmpeg_available(const std::filesystem::path& ffmpeg) {
#ifdef _WIN32
    std::wstring cmd =
        quote_cmdline_arg(std::filesystem::path(path_for_shell(ffmpeg)).wstring()) +
        L" -hide_banner -loglevel quiet -version";
    const FfmpegResult result = run_ffmpeg_windows(cmd);
    return result.exit_code == 0;
#else
    const std::string cmd =
        quote_shell_arg(path_for_shell(ffmpeg)) + " -hide_banner -loglevel quiet -version >/dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
#endif
}

FfmpegResult ffmpeg_convert(const std::filesystem::path& ffmpeg,
                            const std::filesystem::path& input,
                            const std::filesystem::path& output,
                            const std::vector<std::string>& extra_args) {
#ifdef _WIN32
    std::wostringstream cmd;
    cmd << quote_cmdline_arg(std::filesystem::path(path_for_shell(ffmpeg)).wstring())
        << L" -y -hide_banner -loglevel error -i "
        << quote_cmdline_arg(std::filesystem::path(path_for_shell(input)).wstring());
    for (const auto& arg : extra_args) {
        cmd << L' ' << std::wstring(arg.begin(), arg.end());
    }
    cmd << L' ' << quote_cmdline_arg(std::filesystem::path(path_for_shell(output)).wstring());
    return run_ffmpeg_windows(cmd.str());
#else
    std::ostringstream cmd;
    cmd << quote_shell_arg(path_for_shell(ffmpeg)) << " -y -hide_banner -loglevel error -i "
        << quote_shell_arg(path_for_shell(input));
    for (const auto& arg : extra_args) {
        cmd << ' ' << arg;
    }
    cmd << ' ' << quote_shell_arg(path_for_shell(output)) << " 2>&1";

    FfmpegResult result;
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        result.stderr_text = "failed to launch ffmpeg";
        return result;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.stderr_text += buffer;
    }
    result.exit_code = pclose(pipe);
    result.stderr_text = trim_ffmpeg_message(std::move(result.stderr_text));
    return result;
#endif
}

} // namespace d2df::tools
