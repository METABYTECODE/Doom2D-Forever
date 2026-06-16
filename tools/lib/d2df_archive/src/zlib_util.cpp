#include <d2df/tools/zlib_util.hpp>

#include <stdexcept>

#include <zlib.h>

namespace d2df::tools {
namespace {

std::vector<std::uint8_t> inflate_with(z_stream& stream, std::span<const std::uint8_t> compressed,
                                       std::size_t hint_size) {
    std::vector<std::uint8_t> out;
    out.resize(hint_size > 0 ? hint_size : compressed.size() * 4);

    stream.next_in = const_cast<Bytef*>(compressed.data());
    stream.avail_in = static_cast<uInt>(compressed.size());

    int err = Z_OK;
    while (err != Z_STREAM_END) {
        if (stream.total_out >= out.size()) {
            out.resize(out.size() * 2);
        }
        stream.next_out = out.data() + stream.total_out;
        stream.avail_out = static_cast<uInt>(out.size() - stream.total_out);
        err = inflate(&stream, Z_SYNC_FLUSH);
        if (err != Z_OK && err != Z_STREAM_END) {
            inflateEnd(&stream);
            throw std::runtime_error("inflate failed");
        }
    }

    inflateEnd(&stream);
    out.resize(stream.total_out);
    return out;
}

} // namespace

std::vector<std::uint8_t> zlib_inflate_raw(std::span<const std::uint8_t> compressed,
                                           std::size_t hint_size) {
    if (compressed.empty()) {
        return {};
    }

    z_stream stream{};

    if (inflateInit2(&stream, -MAX_WBITS) == Z_OK) {
        try {
            return inflate_with(stream, compressed, hint_size);
        } catch (...) {
            // fall through to zlib wrapper attempt
        }
    }

    stream = z_stream{};
    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("zlib_inflate_raw: inflateInit failed");
    }
    return inflate_with(stream, compressed, hint_size);
}

} // namespace d2df::tools
