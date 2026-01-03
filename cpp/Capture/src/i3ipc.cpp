#include "i3ipc.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace Capture {

I3Scanner::I3Scanner() {
    connectToSocket();
}

I3Scanner::~I3Scanner() {
    if (m_sock != -1) close(m_sock);
}

std::string I3Scanner::getSocketPath() {
    // Check Sway first, then i3
    const char* env = std::getenv("SWAYSOCK");
    if (!env) env = std::getenv("I3SOCK");
    return env ? env : "";
}

bool I3Scanner::connectToSocket() {
    std::string path = getSocketPath();
    if (path.empty()) return false;

    m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_sock == -1) return false;

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(m_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(m_sock);
        m_sock = -1;
        return false;
    }
    return true;
}

void I3Scanner::sendGetTree() {
    if (m_sock == -1) return;

    const std::string magic = "i3-ipc";
    uint32_t payload_len = 0;
    uint32_t type = 4; // GET_TREE

    // 14 byte header
    std::vector<uint8_t> header;
    header.insert(header.end(), magic.begin(), magic.end());
    header.insert(header.end(), (uint8_t*)&payload_len, (uint8_t*)&payload_len + 4);
    header.insert(header.end(), (uint8_t*)&type, (uint8_t*)&type + 4);

    write(m_sock, header.data(), header.size());
}

std::string I3Scanner::receiveResponse() {
    if (m_sock == -1) return "";

    // 1. Read the 14-byte header
    char header[14];
    if (read(m_sock, header, 14) != 14) return "";

    // 2. Extract the length from bytes 6-9 (little endian)
    uint32_t length;
    std::memcpy(&length, &header[6], 4);

    // 3. Read the JSON payload
    std::string json_data;
    json_data.resize(length);
    
    uint32_t total_read = 0;
    while (total_read < length) {
        ssize_t r = read(m_sock, &json_data[total_read], length - total_read);
        if (r <= 0) break;
        total_read += r;
    }
    
    return json_data;
}

// Helper function to search the recursive i3 tree
void find_node_recursive(const nlohmann::json& node, const std::string& name, WindowInfo& info) {
    // Check if this node is our window
    // i3 nodes usually have "name" (title) or "window_properties" -> "class"
    if (node.contains("name") && node["name"].is_string()) {
        if (node["name"].get<std::string>().find(name) != std::string::npos) {
            if (node.contains("window") && !node["window"].is_null()) {
                info.xid = node["window"].get<uint64_t>();
                info.x = node["rect"]["x"];
                info.y = node["rect"]["y"];
                info.w = node["rect"]["width"];
                info.h = node["rect"]["height"];
                info.found = true;
                return;
            }
        }
    }

    // Recurse into children
    if (node.contains("nodes")) {
        for (const auto& child : node["nodes"]) {
            find_node_recursive(child, name, info);
            if (info.found) return;
        }
    }
    if (node.contains("floating_nodes")) {
        for (const auto& child : node["floating_nodes"]) {
            find_node_recursive(child, name, info);
            if (info.found) return;
        }
    }
}

WindowInfo I3Scanner::scanForWindow(const std::string& name) {
    WindowInfo info;
    if (m_sock == -1 && !connectToSocket()) return info;

    sendGetTree();
    std::string json_str = receiveResponse();
    if (json_str.empty()) return info;

    try {
        auto tree = nlohmann::json::parse(json_str);
        find_node_recursive(tree, name, info);
    } catch (...) {
        // Handle parse errors
    }

    return info;
}

} // namespace Capture