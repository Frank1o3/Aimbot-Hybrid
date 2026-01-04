// backends/WaylandBackend.cpp
#include "backends/WaylandBackend.hpp"

#ifdef WAYLAND_BACKEND_ENABLED

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif

namespace Capture {

// Helper function to create memfd (for older systems without memfd_create)
static int create_anonymous_file(off_t size) {
    int fd = syscall(SYS_memfd_create, "wayland-shm", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        std::cerr << "memfd_create failed: " << strerror(errno) << std::endl;
        return -1;
    }
    
    if (ftruncate(fd, size) < 0) {
        std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    
    return fd;
}

// Registry listeners
static const struct wl_registry_listener registry_listener = {
    WaylandBackend::registry_global,
    WaylandBackend::registry_global_remove
};

// Screencopy frame listeners
static const struct zwlr_screencopy_frame_v1_listener frame_listener = {
    WaylandBackend::frame_buffer,
    WaylandBackend::frame_flags,
    WaylandBackend::frame_ready,
    WaylandBackend::frame_failed,
    nullptr, // damage (optional)
    nullptr  // linux_dmabuf (optional)
};

WaylandBackend::WaylandBackend() {
    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        std::cerr << "Failed to connect to Wayland display" << std::endl;
        return;
    }

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registry_listener, this);
    
    // Roundtrip to get globals
    wl_display_roundtrip(m_display);
    
    if (!m_screencopy_manager) {
        std::cerr << "Compositor does not support wlr-screencopy-unstable-v1" << std::endl;
        std::cerr << "This is typically only available on wlroots-based compositors (Sway, Hyprland, etc.)" << std::endl;
    }
}

WaylandBackend::~WaylandBackend() {
    cleanup();
}

void WaylandBackend::cleanup() {
    if (m_frame) {
        zwlr_screencopy_frame_v1_destroy(m_frame);
        m_frame = nullptr;
    }
    
    if (m_wl_buffer) {
        wl_buffer_destroy(m_wl_buffer);
        m_wl_buffer = nullptr;
    }
    
    if (m_shm_data && m_shm_size > 0) {
        munmap(m_shm_data, m_shm_size);
        m_shm_data = nullptr;
    }
    
    if (m_shm_fd >= 0) {
        close(m_shm_fd);
        m_shm_fd = -1;
    }
    
    if (m_screencopy_manager) {
        zwlr_screencopy_manager_v1_destroy(m_screencopy_manager);
        m_screencopy_manager = nullptr;
    }
    
    if (m_output) {
        wl_output_destroy(m_output);
        m_output = nullptr;
    }
    
    if (m_shm) {
        wl_shm_destroy(m_shm);
        m_shm = nullptr;
    }
    
    if (m_registry) {
        wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }
    
    if (m_display) {
        wl_display_disconnect(m_display);
        m_display = nullptr;
    }
}

bool WaylandBackend::init(uint64_t xid, int w, int h) {
    if (!m_display) {
        std::cerr << "Wayland display not connected" << std::endl;
        return false;
    }
    
    if (!m_screencopy_manager) {
        std::cerr << "wlr-screencopy not available" << std::endl;
        return false;
    }
    
    if (!m_shm) {
        std::cerr << "wl_shm not available" << std::endl;
        return false;
    }
    
    m_width = w;
    m_height = h;
    m_stride = w * 4; // RGBA8
    
    std::cout << "Initializing Wayland backend for " << w << "x" << h << std::endl;
    
    return setupSharedMemory(w, h, m_stride);
}

bool WaylandBackend::setupSharedMemory(uint32_t width, uint32_t height, uint32_t stride) {
    m_shm_size = stride * height;
    
    // Create shared memory file
    m_shm_fd = create_anonymous_file(m_shm_size);
    if (m_shm_fd < 0) {
        return false;
    }
    
    // Map memory
    m_shm_data = mmap(nullptr, m_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
    if (m_shm_data == MAP_FAILED) {
        std::cerr << "mmap failed: " << strerror(errno) << std::endl;
        close(m_shm_fd);
        m_shm_fd = -1;
        return false;
    }
    
    // Create wl_shm_pool and buffer
    struct wl_shm_pool* pool = wl_shm_create_pool(m_shm, m_shm_fd, m_shm_size);
    if (!pool) {
        std::cerr << "Failed to create wl_shm_pool" << std::endl;
        munmap(m_shm_data, m_shm_size);
        close(m_shm_fd);
        m_shm_fd = -1;
        return false;
    }
    
    m_wl_buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    
    if (!m_wl_buffer) {
        std::cerr << "Failed to create wl_buffer" << std::endl;
        return false;
    }
    
    std::cout << "Shared memory buffer created successfully" << std::endl;
    return true;
}

bool WaylandBackend::capture(IMGBuffer::Buffer& outBuffer) {
    if (!m_screencopy_manager || !m_wl_buffer) {
        return false;
    }
    
    // Create a new frame for capturing
    // Note: For window capture, you'd need the specific output and region
    // This captures the entire output (screen)
    m_frame = zwlr_screencopy_manager_v1_capture_output(m_screencopy_manager, 0, m_output);
    if (!m_frame) {
        std::cerr << "Failed to create screencopy frame" << std::endl;
        return false;
    }
    
    zwlr_screencopy_frame_v1_add_listener(m_frame, &frame_listener, this);
    zwlr_screencopy_frame_v1_copy(m_frame, m_wl_buffer);
    
    m_ready = false;
    m_failed = false;
    
    // Event loop - wait for frame to be ready
    while (!m_ready && !m_failed && wl_display_dispatch(m_display) != -1) {
        // Continue processing events
    }
    
    if (m_failed) {
        std::cerr << "Frame capture failed" << std::endl;
        return false;
    }
    
    if (m_ready && m_shm_data) {
        // Copy from shared memory to output buffer
        std::memcpy(outBuffer.data(), m_shm_data, std::min(m_shm_size, (size_t)(outBuffer.stride() * outBuffer.height())));
        return true;
    }
    
    return false;
}

uint64_t WaylandBackend::findWindow(const std::string& name) {
    // Window discovery is handled by i3ipc for all backends
    // Sway uses the same IPC protocol as i3
    // This function is kept for interface compatibility
    return 0;
}

// Static callback implementations
void WaylandBackend::registry_global(void* data, struct wl_registry* registry,
                                     uint32_t name, const char* interface, uint32_t version) {
    auto* backend = static_cast<WaylandBackend*>(data);
    
    if (strcmp(interface, wl_shm_interface.name) == 0) {
        backend->m_shm = static_cast<struct wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
        std::cout << "Bound to wl_shm" << std::endl;
    } 
    else if (strcmp(interface, wl_output_interface.name) == 0) {
        backend->m_output = static_cast<struct wl_output*>(
            wl_registry_bind(registry, name, &wl_output_interface, 1));
        std::cout << "Bound to wl_output" << std::endl;
    }
    else if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0) {
        backend->m_screencopy_manager = static_cast<struct zwlr_screencopy_manager_v1*>(
            wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface, 3));
        std::cout << "Bound to zwlr_screencopy_manager_v1" << std::endl;
    }
}

void WaylandBackend::registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    // Handle global removal if needed
}

void WaylandBackend::frame_buffer(void* data, struct zwlr_screencopy_frame_v1* frame,
                                  uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
    // Frame buffer format received
    std::cout << "Frame buffer: " << width << "x" << height << " stride=" << stride << std::endl;
}

void WaylandBackend::frame_flags(void* data, struct zwlr_screencopy_frame_v1* frame, uint32_t flags) {
    // Flags received (e.g., y-invert)
}

void WaylandBackend::frame_ready(void* data, struct zwlr_screencopy_frame_v1* frame,
                                uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec) {
    auto* backend = static_cast<WaylandBackend*>(data);
    backend->m_ready = true;
    
    std::cout << "Frame ready" << std::endl;
    
    zwlr_screencopy_frame_v1_destroy(frame);
    backend->m_frame = nullptr;
}

void WaylandBackend::frame_failed(void* data, struct zwlr_screencopy_frame_v1* frame) {
    auto* backend = static_cast<WaylandBackend*>(data);
    backend->m_failed = true;
    
    std::cerr << "Frame capture failed!" << std::endl;
    
    zwlr_screencopy_frame_v1_destroy(frame);
    backend->m_frame = nullptr;
}

} // namespace Capture

#endif // WAYLAND_BACKEND_ENABLED