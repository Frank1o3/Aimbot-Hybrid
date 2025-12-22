#include "capture.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace capture::xwayland
{

    WaylandCapture::WaylandCapture()
        : width_(0), height_(0), shm_fd_(-1), shm_data_(nullptr), shm_size_(0)
    {
    }

    WaylandCapture::~WaylandCapture()
    {
        cleanup();
    }

    bool WaylandCapture::setup(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return false;
        }

        cleanup();

        width_ = width;
        height_ = height;
        shm_size_ = width * height * 4; // BGRA

        return init_shm();
    }

    bool WaylandCapture::init_shm()
    {
        // Create anonymous file
        shm_fd_ = memfd_create("wl_shm", MFD_CLOEXEC);
        if (shm_fd_ < 0)
        {
            std::cerr << "Failed to create memfd" << std::endl;
            return false;
        }

        // Set size
        if (ftruncate(shm_fd_, shm_size_) < 0)
        {
            std::cerr << "Failed to ftruncate" << std::endl;
            close(shm_fd_);
            shm_fd_ = -1;
            return false;
        }

        // Map memory
        shm_data_ = mmap(nullptr, shm_size_, PROT_READ | PROT_WRITE,
                         MAP_SHARED, shm_fd_, 0);

        if (shm_data_ == MAP_FAILED)
        {
            std::cerr << "Failed to mmap" << std::endl;
            close(shm_fd_);
            shm_fd_ = -1;
            shm_data_ = nullptr;
            return false;
        }

        return true;
    }

    std::vector<uint8_t> WaylandCapture::capture_frame()
    {
        if (!shm_data_)
        {
            throw std::runtime_error("Capture not initialized");
        }

        // Copy from shared memory
        std::vector<uint8_t> frame(shm_size_);
        std::memcpy(frame.data(), shm_data_, shm_size_);

        return frame;
    }

    void WaylandCapture::cleanup()
    {
        if (shm_data_ && shm_data_ != MAP_FAILED)
        {
            munmap(shm_data_, shm_size_);
            shm_data_ = nullptr;
        }

        if (shm_fd_ >= 0)
        {
            close(shm_fd_);
            shm_fd_ = -1;
        }

        width_ = 0;
        height_ = 0;
        shm_size_ = 0;
    }

} // namespace capture::xwayland