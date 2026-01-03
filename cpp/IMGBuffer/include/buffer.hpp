#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace IMGBuffer {

class Buffer {
public:
    Buffer(std::size_t width, std::size_t height);

    void resize(std::size_t width, std::size_t height);

    std::size_t width() const noexcept;
    std::size_t height() const noexcept;
    std::size_t stride() const noexcept;

    std::uint8_t* data() noexcept;
    const std::uint8_t* data() const noexcept;

private:
    std::size_t m_width{};
    std::size_t m_height{};
    std::size_t m_stride{};
    std::vector<std::uint8_t> m_data;
};

} // namespace IMGBuffer
