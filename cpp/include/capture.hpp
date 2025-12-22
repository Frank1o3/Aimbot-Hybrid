#pragma once

#include <vector>
#include <cstdint>

namespace capture::xwayland {

class WaylandCapture {
public:
    WaylandCapture();
    ~WaylandCapture();

    bool setup(int width, int height);
    std::vector<uint8_t> capture_frame();
    void cleanup();

    int width() const { return width_; }
    int height() const { return height_; }

private:
    bool init_shm();

    int width_;
    int height_;
    int shm_fd_;
    void* shm_data_;
    size_t shm_size_;
};

} // namespace capture::xwayland
