#pragma once
#include <buffer.hpp>
#include <string>
#include <cstdint>

namespace Capture {

class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;
    
    /**
     * @brief Initializes the backend for a specific window
     * @param xid The X11 Window ID (found by the scanner)
     * @param w Target width
     * @param h Target height
     */
    virtual bool init(uint64_t xid, int w, int h) = 0;
    
    /**
     * @brief Performs the actual capture from the window into the buffer
     */
    virtual bool capture(IMGBuffer::Buffer& outBuffer) = 0;

    /** * Optional: If you want the backend to be able to find its own window
     */
    virtual uint64_t findWindow(const std::string& name) = 0;
};

} // namespace Capture