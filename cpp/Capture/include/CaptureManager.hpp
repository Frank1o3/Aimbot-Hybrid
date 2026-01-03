#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include "i3ipc.hpp"
#include "ICaptureBackend.hpp"

namespace Capture {

class CaptureManager {
public:
    CaptureManager(const std::string& windowName);
    ~CaptureManager();

    // Controls
    bool init();        // Finds window and allocates memory
    void start();       // Starts the background capture thread
    void stop();        // Stops the thread

    // Data Access for Python
    uint64_t getFrameCount() const { return m_frameCount.load(std::memory_order_relaxed); }
    IMGBuffer::Buffer& getActiveBuffer();

private:
    void captureLoop(); // The function running in the background thread

    std::string m_windowName;
    std::unique_ptr<ICaptureBackend> m_backend;
    
    // Triple buffering logic
    std::vector<std::unique_ptr<IMGBuffer::Buffer>> m_buffers;
    std::atomic<int> m_latestIdx{0}; // Index of the buffer Python should read
    
    // Threading
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_frameCount{0};
};

} // namespace Capture