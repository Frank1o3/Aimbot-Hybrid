#pragma once
#include "ICaptureBackend.hpp"

#ifdef WAYLAND_BACKEND_ENABLED

#include <wayland-client.h>
#include "wlr-screencopy-unstable-v1-client-protocol.h"
#include <string>
#include <cstdint>

namespace Capture {

class WaylandBackend : public ICaptureBackend {
public:
    WaylandBackend();
    ~WaylandBackend() override;

    bool init(uint64_t xid, int w, int h) override;
    bool capture(IMGBuffer::Buffer& outBuffer) override;
    uint64_t findWindow(const std::string& name) override;

private:
    struct wl_display* m_display = nullptr;
    struct wl_registry* m_registry = nullptr;
    struct wl_shm* m_shm = nullptr;
    struct wl_output* m_output = nullptr;
    
    // For screencopy protocol (wlr-screencopy-unstable-v1)
    struct zwlr_screencopy_manager_v1* m_screencopy_manager = nullptr;
    struct zwlr_screencopy_frame_v1* m_frame = nullptr;
    
    int m_width = 0;
    int m_height = 0;
    int m_stride = 0;
    bool m_ready = false;
    bool m_failed = false;
    
    // Shared memory buffer
    int m_shm_fd = -1;
    void* m_shm_data = nullptr;
    size_t m_shm_size = 0;
    struct wl_buffer* m_wl_buffer = nullptr;

    void cleanup();
    bool setupSharedMemory(uint32_t width, uint32_t height, uint32_t stride);
    
    // Wayland callbacks
    static void registry_global(void* data, struct wl_registry* registry,
                               uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name);
    
    static void frame_buffer(void* data, struct zwlr_screencopy_frame_v1* frame,
                            uint32_t format, uint32_t width, uint32_t height, uint32_t stride);
    static void frame_flags(void* data, struct zwlr_screencopy_frame_v1* frame, uint32_t flags);
    static void frame_ready(void* data, struct zwlr_screencopy_frame_v1* frame,
                           uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec);
    static void frame_failed(void* data, struct zwlr_screencopy_frame_v1* frame);
};

} // namespace Capture

#endif // WAYLAND_BACKEND_ENABLED