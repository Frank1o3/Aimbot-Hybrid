#pragma once
#include "ICaptureBackend.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <string>

namespace Capture {

class XWaylandBackend {
public:
    XWaylandBackend();
    ~XWaylandBackend();

    // ADD THIS LINE:
    bool init(uint64_t xid, int w, int h);
    
    // Ensure these match the interface as well
    uint64_t findWindow(const std::string& name);
    bool capture(IMGBuffer::Buffer& outBuffer);

private:
    Display* m_display = nullptr;
    Window m_targetWindow = 0;
    XImage* m_xImage = nullptr;
    XShmSegmentInfo m_shmInfo;
    bool m_shmAttached = false;

    void cleanup();
};

} // namespace Capture