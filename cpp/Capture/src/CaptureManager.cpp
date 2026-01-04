#include "CaptureManager.hpp"
#include "backends/XWaylandBackend.hpp"

#ifdef WAYLAND_BACKEND_ENABLED
#include "backends/WaylandBackend.hpp"
#endif

#include <iostream>
#include <cstdlib>

namespace Capture {

CaptureManager::CaptureManager(const std::string& windowName, BackendType backend) 
    : m_windowName(windowName), m_backendType(backend) {}

CaptureManager::~CaptureManager() {
    stop();
}

BackendType CaptureManager::detectBackend(const WindowInfo& windowInfo) {
    // If the window has an X11 ID (xid), we should use XWaylandBackend
    // If it doesn't have an X11 ID (xid == 0), it's a native Wayland window
    if (windowInfo.xid != 0) {
        std::cout << "Window has X11 ID - using XWayland/X11 backend" << std::endl;
        #ifdef X11_BACKEND_ENABLED
        return BackendType::XWayland;
        #else
        std::cerr << "X11 backend not available but window has X11 ID!" << std::endl;
        return BackendType::Auto; // Will fail later
        #endif
    }
    
    // Native Wayland window
    if (windowInfo.server == DisplayServer::Sway) {
        std::cout << "Native Wayland window detected - using Wayland backend" << std::endl;
        #ifdef WAYLAND_BACKEND_ENABLED
        return BackendType::Wayland;
        #else
        std::cerr << "Wayland backend not compiled! Cannot capture native Wayland windows." << std::endl;
        std::cerr << "Rebuild with libwayland-dev installed." << std::endl;
        return BackendType::Auto; // Will fail later
        #endif
    }
    
    // Fallback based on environment
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    const char* x11Display = std::getenv("DISPLAY");
    
    if (x11Display) {
        std::cout << "DISPLAY set - defaulting to XWayland/X11 backend" << std::endl;
        #ifdef X11_BACKEND_ENABLED
        return BackendType::XWayland;
        #else
        std::cerr << "X11 backend not available" << std::endl;
        #endif
    }
    
    if (waylandDisplay) {
        std::cout << "WAYLAND_DISPLAY set - defaulting to Wayland backend" << std::endl;
        #ifdef WAYLAND_BACKEND_ENABLED
        return BackendType::Wayland;
        #else
        std::cerr << "Wayland backend not available" << std::endl;
        #ifdef X11_BACKEND_ENABLED
        return BackendType::XWayland; // Fallback
        #endif
        #endif
    }
    
    // Ultimate fallback - prefer X11 if available
    #ifdef X11_BACKEND_ENABLED
    std::cout << "Could not detect environment - defaulting to XWayland/X11" << std::endl;
    return BackendType::XWayland;
    #elif defined(WAYLAND_BACKEND_ENABLED)
    std::cout << "Could not detect environment - defaulting to Wayland" << std::endl;
    return BackendType::Wayland;
    #else
    std::cerr << "No backends available!" << std::endl;
    return BackendType::Auto; // Will fail
    #endif
}

std::unique_ptr<ICaptureBackend> CaptureManager::createBackend(BackendType type) {
    switch (type) {
        case BackendType::XWayland:
            #ifdef X11_BACKEND_ENABLED
            std::cout << "Creating XWayland/X11 backend" << std::endl;
            return std::make_unique<XWaylandBackend>();
            #else
            std::cerr << "X11 backend not compiled!" << std::endl;
            std::cerr << "Install libx11-dev and libxext-dev, then rebuild" << std::endl;
            return nullptr;
            #endif
        
        case BackendType::Wayland:
            #ifdef WAYLAND_BACKEND_ENABLED
            std::cout << "Creating Wayland backend" << std::endl;
            return std::make_unique<WaylandBackend>();
            #else
            std::cerr << "Wayland backend not compiled!" << std::endl;
            std::cerr << "Install libwayland-dev, then rebuild" << std::endl;
            #ifdef X11_BACKEND_ENABLED
            std::cerr << "Falling back to XWayland/X11 backend" << std::endl;
            return std::make_unique<XWaylandBackend>();
            #else
            return nullptr;
            #endif
            #endif
        
        case BackendType::Auto:
            // This shouldn't happen as Auto should be resolved in init
            std::cerr << "Auto backend should have been resolved!" << std::endl;
            #ifdef X11_BACKEND_ENABLED
            return std::make_unique<XWaylandBackend>();
            #elif defined(WAYLAND_BACKEND_ENABLED)
            return std::make_unique<WaylandBackend>();
            #else
            return nullptr;
            #endif
        
        default:
            return nullptr;
    }
}

void CaptureManager::setBackend(BackendType backend) {
    if (m_running) {
        std::cerr << "Cannot change backend while capture is running" << std::endl;
        return;
    }
    
    m_backendType = backend;
    m_backend.reset(); // Clear existing backend
}

std::string CaptureManager::getBackendName() const {
    switch (m_backendType) {
        case BackendType::Auto: return "Auto";
        case BackendType::XWayland: return "XWayland/X11";
        case BackendType::Wayland: return "Wayland";
        default: return "Unknown";
    }
}

bool CaptureManager::init() {
    std::cout << "=== CaptureManager Initialization ===" << std::endl;
    
    // Step 1: Find the window using i3ipc (works for all backends)
    I3Scanner scanner;
    WindowInfo info = scanner.scanForWindow(m_windowName);

    if (!info.found) {
        std::cerr << "Could not find window: " << m_windowName << std::endl;
        return false;
    }

    std::cout << "Window found successfully!" << std::endl;

    // Step 2: Auto-detect backend if needed
    if (m_backendType == BackendType::Auto) {
        m_backendType = detectBackend(info);
        std::cout << "Auto-detected backend: " << getBackendName() << std::endl;
    } else {
        std::cout << "Using specified backend: " << getBackendName() << std::endl;
    }

    // Step 3: Create the appropriate backend
    m_backend = createBackend(m_backendType);
    if (!m_backend) {
        std::cerr << "Failed to create backend: " << getBackendName() << std::endl;
        return false;
    }

    // Step 4: Initialize the backend with window info
    // For native Wayland windows without X11 ID, we pass 0
    // The backend will need to use other mechanisms (output capture, etc.)
    if (!m_backend->init(info.xid, info.w, info.h)) {
        std::cerr << "Backend initialization failed" << std::endl;
        return false;
    }

    // Step 5: Pre-allocate 3 buffers for Triple Buffering
    std::cout << "Allocating triple buffers (" << info.w << "x" << info.h << ")" << std::endl;
    for (int i = 0; i < 3; ++i) {
        m_buffers.push_back(std::make_unique<IMGBuffer::Buffer>(info.w, info.h));
    }

    std::cout << "CaptureManager initialized successfully!" << std::endl;
    std::cout << "=====================================" << std::endl;
    return true;
}

void CaptureManager::start() {
    if (m_running) {
        std::cout << "Capture already running" << std::endl;
        return;
    }
    
    std::cout << "Starting capture thread..." << std::endl;
    m_running = true;
    m_worker = std::thread(&CaptureManager::captureLoop, this);
}

void CaptureManager::stop() {
    if (!m_running) {
        return;
    }
    
    std::cout << "Stopping capture thread..." << std::endl;
    m_running = false;
    
    if (m_worker.joinable()) {
        m_worker.join();
    }
    
    std::cout << "Capture thread stopped. Total frames: " << m_frameCount << std::endl;
}

void CaptureManager::captureLoop() {
    int writeIdx = 1; // Start writing to the first "back" buffer
    uint64_t failCount = 0;

    std::cout << "Capture loop started" << std::endl;

    while (m_running.load(std::memory_order_relaxed)) {
        // 1. Capture into the current 'write' buffer
        if (m_backend->capture(*m_buffers[writeIdx])) {
            // Reset fail counter on success
            failCount = 0;
            
            // 2. Atomic Swap: Make this buffer the "Latest" for Python to see
            // Use 'release' to ensure the pixels are in RAM before updating the index
            int oldLatest = m_latestIdx.exchange(writeIdx, std::memory_order_release);
            
            // 3. The old latest buffer is now our new 'write' buffer (recycling)
            writeIdx = oldLatest;

            // 4. Increment frame count for Python to check for updates
            m_frameCount.fetch_add(1, std::memory_order_relaxed);
        } else {
            // Capture failed
            failCount++;
            
            if (failCount % 100 == 1) {
                std::cerr << "Capture failing (count: " << failCount << ")" << std::endl;
            }
            
            // Back off if we're failing repeatedly
            if (failCount > 10) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        // Small yield to prevent 100% CPU usage
        std::this_thread::yield();
    }
    
    std::cout << "Capture loop exited" << std::endl;
}

IMGBuffer::Buffer& CaptureManager::getActiveBuffer() {
    // Use 'acquire' to ensure we see the pixels the thread just finished writing
    int idx = m_latestIdx.load(std::memory_order_acquire);
    return *m_buffers[idx];
}

} // namespace Capture