#include "i3ipc.hpp"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

namespace Capture {

I3Scanner::I3Scanner() {
    m_detectedServer = detectDisplayServer();
    connectToSocket();
}

I3Scanner::~I3Scanner() {
    if (m_sock != -1) close(m_sock);
}

DisplayServer I3Scanner::detectDisplayServer() {
    // Check environment variables
    const char* swaysock = std::getenv("SWAYSOCK");
    const char* i3sock = std::getenv("I3SOCK");
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    const char* x11Display = std::getenv("DISPLAY");
    
    // Prioritize Sway
    if (swaysock) {
        std::cout << "Detected Sway compositor" << std::endl;
        return DisplayServer::Sway;
    }
    
    // Then i3
    if (i3sock) {
        std::cout << "Detected i3 window manager" << std::endl;
        return DisplayServer::I3;
    }
    
    // Check display server type
    if (waylandDisplay && x11Display) {
        std::cout << "Detected XWayland environment" << std::endl;
        return DisplayServer::XWayland;
    }
    
    if (x11Display) {
        std::cout << "Detected X11 environment" << std::endl;
        return DisplayServer::X11;
    }
    
    return DisplayServer::Unknown;
}

std::string I3Scanner::getSocketPath() {
    // Check Sway first, then i3
    const char* env = std::getenv("SWAYSOCK");
    if (!env) env = std::getenv("I3SOCK");
    return env ? env : "";
}

bool I3Scanner::connectToSocket() {
    std::string path = getSocketPath();
    if (path.empty()) {
        std::cerr << "Could not find i3/Sway socket path" << std::endl;
        return false;
    }

    m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_sock == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(m_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to i3/Sway socket" << std::endl;
        close(m_sock);
        m_sock = -1;
        return false;
    }
    
    std::cout << "Connected to i3/Sway socket: " << path << std::endl;
    return true;
}

void I3Scanner::sendGetTree() {
    if (m_sock == -1) return;

    const std::string magic = "i3-ipc";
    uint32_t payload_len = 0;
    uint32_t type = 4; // GET_TREE

    // Build 14 byte header
    std::vector<uint8_t> header;
    header.insert(header.end(), magic.begin(), magic.end());
    header.insert(header.end(), (uint8_t*)&payload_len, (uint8_t*)&payload_len + 4);
    header.insert(header.end(), (uint8_t*)&type, (uint8_t*)&type + 4);

    write(m_sock, header.data(), header.size());
}

std::string I3Scanner::receiveResponse() {
    if (m_sock == -1) return "";

    // Read the 14-byte header
    char header[14];
    if (read(m_sock, header, 14) != 14) {
        std::cerr << "Failed to read i3-ipc header" << std::endl;
        return "";
    }

    // Extract the payload length from bytes 6-9 (little endian)
    uint32_t length;
    std::memcpy(&length, &header[6], 4);

    // Read the JSON payload
    std::string json_data;
    json_data.resize(length);
    
    uint32_t total_read = 0;
    while (total_read < length) {
        ssize_t r = read(m_sock, &json_data[total_read], length - total_read);
        if (r <= 0) {
            std::cerr << "Failed to read i3-ipc payload" << std::endl;
            break;
        }
        total_read += r;
    }
    
    return json_data;
}

// Recursive function to find window in tree
void find_node_recursive(const nlohmann::json& node, const std::string& name, 
                        WindowInfo& info, DisplayServer server) {
    // Check if this node matches our search
    if (node.contains("name") && node["name"].is_string()) {
        std::string nodeName = node["name"].get<std::string>();
        
        if (nodeName.find(name) != std::string::npos) {
            // For X11/XWayland windows, we need the 'window' field (X11 window ID)
            if (node.contains("window") && !node["window"].is_null()) {
                info.xid = node["window"].get<uint64_t>();
                info.x = node["rect"]["x"].get<int>();
                info.y = node["rect"]["y"].get<int>();
                info.w = node["rect"]["width"].get<int>();
                info.h = node["rect"]["height"].get<int>();
                info.found = true;
                info.server = server;
                
                // Get app_id for Wayland windows
                if (node.contains("app_id") && node["app_id"].is_string()) {
                    info.app_id = node["app_id"].get<std::string>();
                }
                
                std::cout << "Found window: '" << nodeName << "'" << std::endl;
                std::cout << "  XID: " << info.xid << std::endl;
                std::cout << "  Position: (" << info.x << ", " << info.y << ")" << std::endl;
                std::cout << "  Size: " << info.w << "x" << info.h << std::endl;
                if (!info.app_id.empty()) {
                    std::cout << "  App ID: " << info.app_id << std::endl;
                }
                return;
            }
            // For pure Wayland windows (no X11 window ID)
            else if (server == DisplayServer::Sway) {
                std::cout << "Found Wayland-native window (no X11 ID): '" << nodeName << "'" << std::endl;
                info.xid = 0; // No X11 ID for native Wayland windows
                info.x = node["rect"]["x"].get<int>();
                info.y = node["rect"]["y"].get<int>();
                info.w = node["rect"]["width"].get<int>();
                info.h = node["rect"]["height"].get<int>();
                info.found = true;
                info.server = server;
                
                if (node.contains("app_id") && node["app_id"].is_string()) {
                    info.app_id = node["app_id"].get<std::string>();
                    std::cout << "  App ID: " << info.app_id << std::endl;
                }
                
                std::cout << "  Position: (" << info.x << ", " << info.y << ")" << std::endl;
                std::cout << "  Size: " << info.w << "x" << info.h << std::endl;
                std::cout << "  Note: This is a native Wayland window" << std::endl;
                return;
            }
        }
    }

    // Recurse into child nodes
    if (node.contains("nodes") && node["nodes"].is_array()) {
        for (const auto& child : node["nodes"]) {
            find_node_recursive(child, name, info, server);
            if (info.found) return;
        }
    }
    
    // Recurse into floating nodes
    if (node.contains("floating_nodes") && node["floating_nodes"].is_array()) {
        for (const auto& child : node["floating_nodes"]) {
            find_node_recursive(child, name, info, server);
            if (info.found) return;
        }
    }
}

WindowInfo I3Scanner::scanForWindow(const std::string& name) {
    WindowInfo info;
    
    if (m_sock == -1 && !connectToSocket()) {
        std::cerr << "Not connected to i3/Sway" << std::endl;
        return info;
    }

    sendGetTree();
    std::string json_str = receiveResponse();
    
    if (json_str.empty()) {
        std::cerr << "Received empty response from i3/Sway" << std::endl;
        return info;
    }

    try {
        auto tree = nlohmann::json::parse(json_str);
        find_node_recursive(tree, name, info, m_detectedServer);
        
        if (!info.found) {
            std::cerr << "Window '" << name << "' not found in tree" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
    }

    return info;
}

} // namespace Capture