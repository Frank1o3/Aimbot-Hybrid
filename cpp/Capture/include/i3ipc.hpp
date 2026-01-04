#pragma once
#include <string>
#include <cstdint>

namespace Capture {

enum class DisplayServer {
    Unknown,
    I3,          // i3 window manager
    Sway,        // Sway (Wayland compositor)
    XWayland,    // XWayland windows on Wayland
    X11          // Pure X11
};

struct WindowInfo {
    uint64_t xid;       // X11 Window ID (0 for pure Wayland windows)
    int x, y, w, h;     // Position and Size
    bool found = false;
    DisplayServer server = DisplayServer::Unknown;
    std::string app_id; // Wayland app_id (if available)
};

class I3Scanner {
public:
    I3Scanner();
    ~I3Scanner();

    /**
     * @brief Scans for a window across i3/Sway
     * Works for both X11 and Wayland windows
     */
    WindowInfo scanForWindow(const std::string& name);
    
    /**
     * @brief Detects which display server we're running on
     */
    DisplayServer detectDisplayServer();

private:
    int m_sock = -1;
    DisplayServer m_detectedServer = DisplayServer::Unknown;
    
    std::string getSocketPath();
    bool connectToSocket();
    void sendGetTree();
    std::string receiveResponse();
};

} // namespace Capture