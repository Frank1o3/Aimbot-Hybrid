#include "buffer.hpp"

namespace IMGBuffer {

Buffer::Buffer(std::size_t width, std::size_t height) {
    resize(width, height);
}

void Buffer::resize(std::size_t width, std::size_t height) {
    m_width = width;
    m_height = height;
    m_stride = width * 4; // RGBA8

    m_data.resize(m_stride * m_height);
}

std::size_t Buffer::width() const noexcept {
    return m_width;
}

std::size_t Buffer::height() const noexcept {
    return m_height;
}

std::size_t Buffer::stride() const noexcept {
    return m_stride;
}

std::uint8_t* Buffer::data() noexcept {
    return m_data.data();
}

const std::uint8_t* Buffer::data() const noexcept {
    return m_data.data();
}

} // namespace IMGBuffer
