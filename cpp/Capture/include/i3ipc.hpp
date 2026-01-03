#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace Capture {

struct WindowInfo {
    uint64_t xid;       // X11 Window ID
    int x, y, w, h;     // Position and Size
    bool found = false;
};

class I3Scanner {
public:
    I3Scanner();
    ~I3Scanner();

    // The main function: Connects, asks for the tree, and finds the window
    WindowInfo scanForWindow(const std::string& name);

private:
    int m_sock = -1;
    std::string getSocketPath();
    bool connectToSocket();
    void sendGetTree();
    std::string receiveResponse();
};

} // namespace Capture