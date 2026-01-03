#pragma once
#include "ICaptureBackend.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>

namespace Capture {

class XWaylandBackend : public ICaptureBackend {
public:
    XWaylandBackend();
    ~XWaylandBackend();

    uint64_t findWindow(const std::string& name) override;
    bool capture(IMGBuffer::Buffer& outBuffer) override;

private:
    Display* m_display = nullptr;
    Window m_targetWindow = 0;
    XImage* m_xImage = nullptr;
    XShmSegmentInfo m_shmInfo;
    bool m_shmAttached = false;

    // The i3ipc socket connection logic
    int connectToI3(); 
    void cleanup();
};

} // namespace Capture