#include "backends/XWaylandBackend.hpp"
#include <iostream>
#include <cstring>
#include <X11/Xutil.h>

namespace Capture {

XWaylandBackend::XWaylandBackend() {
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        std::cerr << "Failed to open X11 display" << std::endl;
    }
}

XWaylandBackend::~XWaylandBackend() {
    cleanup();
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

bool XWaylandBackend::init(uint64_t xid, int w, int h) {
    if (!m_display) {
        std::cerr << "X11 display not available" << std::endl;
        return false;
    }
    
    m_targetWindow = (Window)xid;

    // 1. Check if X Server supports SHM extension
    if (!XShmQueryExtension(m_display)) {
        std::cerr << "XShm extension not supported!" << std::endl;
        std::cerr << "Falling back to non-SHM capture would be very slow." << std::endl;
        return false;
    }

    std::cout << "XShm extension available" << std::endl;

    // 2. Create the XImage structure
    // ZPixmap format gives us the full pixel data in one block
    m_xImage = XShmCreateImage(
        m_display, 
        DefaultVisual(m_display, DefaultScreen(m_display)),
        DefaultDepth(m_display, DefaultScreen(m_display)), 
        ZPixmap, 
        nullptr, 
        &m_shmInfo, 
        w, 
        h
    );

    if (!m_xImage) {
        std::cerr << "Failed to create XShmImage" << std::endl;
        return false;
    }

    std::cout << "Created XImage: " << w << "x" << h 
              << " depth=" << m_xImage->depth 
              << " bpp=" << m_xImage->bits_per_pixel << std::endl;

    // 3. Allocate Linux Shared Memory (IPC)
    size_t shmSize = m_xImage->bytes_per_line * m_xImage->height;
    m_shmInfo.shmid = shmget(IPC_PRIVATE, shmSize, IPC_CREAT | 0777);
    
    if (m_shmInfo.shmid == -1) {
        std::cerr << "Failed to allocate shared memory: " << strerror(errno) << std::endl;
        XDestroyImage(m_xImage);
        m_xImage = nullptr;
        return false;
    }

    // 4. Attach memory to our process address space
    m_shmInfo.shmaddr = (char*)shmat(m_shmInfo.shmid, nullptr, 0);
    if (m_shmInfo.shmaddr == (char*)-1) {
        std::cerr << "Failed to attach shared memory: " << strerror(errno) << std::endl;
        shmctl(m_shmInfo.shmid, IPC_RMID, nullptr);
        XDestroyImage(m_xImage);
        m_xImage = nullptr;
        return false;
    }
    
    m_xImage->data = m_shmInfo.shmaddr;
    m_shmInfo.readOnly = False;

    // 5. Attach memory to the X Server
    if (!XShmAttach(m_display, &m_shmInfo)) {
        std::cerr << "Failed to attach shared memory to X server" << std::endl;
        shmdt(m_shmInfo.shmaddr);
        shmctl(m_shmInfo.shmid, IPC_RMID, nullptr);
        XDestroyImage(m_xImage);
        m_xImage = nullptr;
        return false;
    }
    
    // Wait for the X server to complete the attach
    XSync(m_display, False);
    
    m_shmAttached = true;
    
    std::cout << "XWaylandBackend initialized successfully" << std::endl;
    std::cout << "  Target window: 0x" << std::hex << m_targetWindow << std::dec << std::endl;
    std::cout << "  Shared memory: " << shmSize << " bytes" << std::endl;
    
    return true;
}

bool XWaylandBackend::capture(IMGBuffer::Buffer& outBuffer) {
    if (!m_xImage || !m_shmAttached) {
        return false;
    }

    // High-speed capture using shared memory
    if (!XShmGetImage(m_display, m_targetWindow, m_xImage, 0, 0, AllPlanes)) {
        std::cerr << "XShmGetImage failed" << std::endl;
        return false;
    }

    // Direct memory copy from shared memory to our buffer
    // Note: X11 typically outputs BGRA or BGRX format
    size_t copySize = std::min(
        (size_t)(m_xImage->bytes_per_line * m_xImage->height),
        (size_t)(outBuffer.stride() * outBuffer.height())
    );
    
    std::memcpy(outBuffer.data(), m_xImage->data, copySize);
    
    return true;
}

uint64_t XWaylandBackend::findWindow(const std::string& name) {
    // Window discovery is handled by i3ipc for all backends
    // This function is kept for interface compatibility
    return 0;
}

void XWaylandBackend::cleanup() {
    if (m_shmAttached) {
        XShmDetach(m_display, &m_shmInfo);
        m_shmAttached = false;
    }
    
    if (m_shmInfo.shmaddr) {
        shmdt(m_shmInfo.shmaddr);
        m_shmInfo.shmaddr = nullptr;
    }
    
    if (m_shmInfo.shmid != -1) {
        shmctl(m_shmInfo.shmid, IPC_RMID, nullptr);
        m_shmInfo.shmid = -1;
    }
    
    if (m_xImage) {
        // XDestroyImage will free m_xImage->data if it's allocated
        // But since we're using shared memory, we need to be careful
        m_xImage->data = nullptr; // Prevent double-free
        XDestroyImage(m_xImage);
        m_xImage = nullptr;
    }
}

} // namespace Capture