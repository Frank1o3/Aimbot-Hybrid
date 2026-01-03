#include "backends/XWaylandBackend.hpp"
#include <iostream>
#include <cstring>
#include <X11/Xutil.h>

namespace Capture {

XWaylandBackend::XWaylandBackend() {
    m_display = XOpenDisplay(NULL);
}

XWaylandBackend::~XWaylandBackend() {
    cleanup();
    if (m_display) XCloseDisplay(m_display);
}

bool XWaylandBackend::init(uint64_t xid, int w, int h) {
    if (!m_display) return false;
    m_targetWindow = (Window)xid;

    // 1. Check if X Server supports SHM
    if (!XShmQueryExtension(m_display)) {
        std::cerr << "XShm extension not supported!" << std::endl;
        return false;
    }

    // 2. Create the XImage structure
    // We use ZPixmap to get the full image data in one block
    m_xImage = XShmCreateImage(m_display, DefaultVisual(m_display, 0),
                               DefaultDepth(m_display, 0), ZPixmap, NULL, 
                               &m_shmInfo, w, h);

    if (!m_xImage) return false;

    // 3. Allocate Linux Shared Memory (IPC)
    m_shmInfo.shmid = shmget(IPC_PRIVATE, m_xImage->bytes_per_line * m_xImage->height, IPC_CREAT | 0777);
    if (m_shmInfo.shmid == -1) return false;

    // 4. Attach memory to our process address space
    m_shmInfo.shmaddr = (char*)shmat(m_shmInfo.shmid, 0, 0);
    m_xImage->data = m_shmInfo.shmaddr;
    m_shmInfo.readOnly = False;

    // 5. Attach memory to the X Server
    if (!XShmAttach(m_display, &m_shmInfo)) {
        return false;
    }
    m_shmAttached = true;
    return true;
}

bool XWaylandBackend::capture(IMGBuffer::Buffer& outBuffer) {
    // High-speed capture call
    if (!XShmGetImage(m_display, m_targetWindow, m_xImage, 0, 0, AllPlanes)) {
        return false;
    }

    // Direct memory copy to our IMGBuffer
    // Note: X11 usually outputs BGRA. Your Python side might need to swap channels
    // or you can do it here if CPU allows.
    std::memcpy(outBuffer.data(), m_xImage->data, m_xImage->bytes_per_line * m_xImage->height);
    
    return true;
}

void XWaylandBackend::cleanup() {
    if (m_shmAttached) {
        XShmDetach(m_display, &m_shmInfo);
        shmdt(m_shmInfo.shmaddr);
        shmctl(m_shmInfo.shmid, IPC_RMID, 0);
        m_shmAttached = false;
    }
    if (m_xImage) {
        // This also frees m_xImage->data if we didn't manually detach
        XDestroyImage(m_xImage);
        m_xImage = nullptr;
    }
}

// Inside XWaylandBackend.cpp
uint64_t XWaylandBackend::findWindow(const std::string& name) {
    return 0; // The I3Scanner handles this in the Manager
}

} // namespace Capture