#include "CaptureManager.hpp"
#include "backends/XWaylandBackend.hpp"
#include <iostream>

namespace Capture {

CaptureManager::CaptureManager(const std::string& windowName) 
    : m_windowName(windowName) {}

CaptureManager::~CaptureManager() {
    stop();
}

bool CaptureManager::init() {
    I3Scanner scanner;
    WindowInfo info = scanner.scanForWindow(m_windowName);

    if (!info.found) {
        std::cerr << "Could not find window: " << m_windowName << std::endl;
        return false;
    }

    // 1. Setup the Backend (XWayland)
    m_backend = std::make_unique<XWaylandBackend>();
    if (!m_backend->init(info.xid, info.w, info.h)) return false;

    // 2. Pre-allocate 3 buffers for Triple Buffering
    // This prevents memory allocation during the capture loop
    for (int i = 0; i < 3; ++i) {
        m_buffers.push_back(std::make_unique<IMGBuffer::Buffer>(info.w, info.h));
    }

    return true;
}

void CaptureManager::start() {
    if (m_running) return;
    m_running = true;
    m_worker = std::thread(&CaptureManager::captureLoop, this);
}

void CaptureManager::stop() {
    m_running = false;
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void CaptureManager::captureLoop() {
    int writeIdx = 1; // Start writing to the first "back" buffer

    while (m_running.load(std::memory_order_relaxed)) {
        // 1. Capture into the current 'write' buffer
        if (m_backend->capture(*m_buffers[writeIdx])) {
            
            // 2. Atomic Swap: Make this buffer the "Latest" for Python to see
            // Use 'release' to ensure the pixels are actually in RAM before updating the index
            int oldLatest = m_latestIdx.exchange(writeIdx, std::memory_order_release);
            
            // 3. The old latest buffer is now our new 'write' buffer (recycling)
            writeIdx = oldLatest;

            // 4. Increment frame count for Python to check for updates
            m_frameCount.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Optional: Small yield to prevent 100% CPU usage if no new frames are ready
        std::this_thread::yield();
    }
}

IMGBuffer::Buffer& CaptureManager::getActiveBuffer() {
    // Use 'acquire' to ensure we see the pixels the thread just finished writing
    int idx = m_latestIdx.load(std::memory_order_acquire);
    return *m_buffers[idx];
}

} // namespace Capture