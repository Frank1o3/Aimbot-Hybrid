#pragma once
#include <buffer.hpp>
#include <string>

namespace Capture {

class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;
    
    // Returns the window ID found via i3ipc
    virtual uint64_t findWindow(const std::string& name) = 0;
    
    // Performs the actual XShm capture
    virtual bool capture(IMGBuffer::Buffer& outBuffer) = 0;
};

} // namespace Capture