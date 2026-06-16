#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace d2df::tools {

class BinaryReader {
public:
    explicit BinaryReader(std::span<const std::uint8_t> data);

    [[nodiscard]] std::size_t position() const { return pos_; }
    [[nodiscard]] std::size_t remaining() const { return data_.size() - pos_; }
    [[nodiscard]] bool eof() const { return pos_ >= data_.size(); }

    void seek(std::size_t pos);
    void skip(std::size_t count);

    std::uint8_t read_u8();
    std::uint16_t read_u16_le();
    std::uint32_t read_u32_le();

    std::vector<std::uint8_t> read_bytes(std::size_t count);
    std::string read_c_string(std::size_t max_len);

private:
    std::span<const std::uint8_t> data_;
    std::size_t pos_ = 0;
};

void read_exact(std::span<const std::uint8_t> data, std::size_t offset, std::span<std::uint8_t> out);

} // namespace d2df::tools
