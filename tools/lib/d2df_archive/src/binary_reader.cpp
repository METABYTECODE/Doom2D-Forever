#include <d2df/tools/binary_reader.hpp>

#include <cstring>

namespace d2df::tools {

BinaryReader::BinaryReader(std::span<const std::uint8_t> data) : data_(data) {}

void BinaryReader::seek(std::size_t pos) {
    if (pos > data_.size()) {
        throw std::runtime_error("BinaryReader: seek past end");
    }
    pos_ = pos;
}

void BinaryReader::skip(std::size_t count) {
    seek(pos_ + count);
}

std::uint8_t BinaryReader::read_u8() {
    if (pos_ >= data_.size()) {
        throw std::runtime_error("BinaryReader: unexpected EOF");
    }
    return data_[pos_++];
}

std::uint16_t BinaryReader::read_u16_le() {
    const auto lo = read_u8();
    const auto hi = read_u8();
    return static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi) << 8));
}

std::uint32_t BinaryReader::read_u32_le() {
    const auto a = read_u8();
    const auto b = read_u8();
    const auto c = read_u8();
    const auto d = read_u8();
    return static_cast<std::uint32_t>(a | (static_cast<std::uint32_t>(b) << 8) |
                                      (static_cast<std::uint32_t>(c) << 16) |
                                      (static_cast<std::uint32_t>(d) << 24));
}

std::vector<std::uint8_t> BinaryReader::read_bytes(std::size_t count) {
    if (pos_ + count > data_.size()) {
        throw std::runtime_error("BinaryReader: read past end");
    }
    std::vector<std::uint8_t> out(count);
    std::memcpy(out.data(), data_.data() + pos_, count);
    pos_ += count;
    return out;
}

std::string BinaryReader::read_c_string(std::size_t max_len) {
    std::string out;
    out.reserve(max_len);
    for (std::size_t i = 0; i < max_len; ++i) {
        const char c = static_cast<char>(read_u8());
        if (c == '\0') {
            break;
        }
        out.push_back(c);
    }
    if (remaining() > 0 && max_len > out.size()) {
        skip(max_len - out.size() - 1);
    }
    return out;
}

void read_exact(std::span<const std::uint8_t> data, std::size_t offset, std::span<std::uint8_t> out) {
    if (offset + out.size() > data.size()) {
        throw std::runtime_error("read_exact: out of range");
    }
    std::memcpy(out.data(), data.data() + offset, out.size());
}

} // namespace d2df::tools
