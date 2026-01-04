#pragma once
#include "ICaptureBackend.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <string>

namespace Capture {

/**
 * @brief Backend for capturing X11 windows (works on both X11 and XWayland)
 * 
 * Uses X11 Shared Memory extension for high-performance screen capture.
 * Works with:
 * - Pure X11 environments
 * - XWayland (X11 apps running on Wayland)
 */
class XWaylandBackend : public ICaptureBackend {
public:
    XWaylandBackend();
    ~XWaylandBackend() override;

    bool init(uint64_t xid, int w, int h) override;
    bool capture(IMGBuffer::Buffer& outBuffer) override;
    uint64_t findWindow(const std::string& name) override;

private:
    Display* m_display = nullptr;
    Window m_targetWindow = 0;
    XImage* m_xImage = nullptr;
    XShmSegmentInfo m_shmInfo{};
    bool m_shmAttached = false;

    void cleanup();
};

} // namespace Capture