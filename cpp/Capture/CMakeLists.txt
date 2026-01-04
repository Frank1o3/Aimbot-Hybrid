# cpp/Capture/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# ============================================================================
# X11 Dependencies
# ============================================================================
pkg_check_modules(X11 QUIET x11 xext)

if(NOT X11_FOUND)
    message(WARNING "X11 libraries not found on system. X11/XWayland backend will be disabled.")
    message(WARNING "To enable X11 support, install: sudo apt install libx11-dev libxext-dev")
    set(X11_BACKEND_ENABLED FALSE)
else()
    message(STATUS "Found X11 libraries")
    set(X11_BACKEND_ENABLED TRUE)
endif()

# ============================================================================
# Wayland Dependencies
# ============================================================================
pkg_check_modules(WAYLAND QUIET wayland-client)

if(NOT WAYLAND_FOUND)
    message(WARNING "Wayland libraries not found on system. Wayland backend will be disabled.")
    message(WARNING "To enable Wayland support, install: sudo apt install libwayland-dev wayland-protocols")
    set(WAYLAND_BACKEND_ENABLED FALSE)
else()
    message(STATUS "Found Wayland libraries")
    
    # Find wayland-scanner
    pkg_check_modules(WAYLAND_SCANNER QUIET wayland-scanner)
    if(WAYLAND_SCANNER_FOUND)
        pkg_get_variable(WAYLAND_SCANNER_BIN wayland-scanner wayland_scanner)
        message(STATUS "Found wayland-scanner: ${WAYLAND_SCANNER_BIN}")
    else()
        find_program(WAYLAND_SCANNER_BIN wayland-scanner)
        if(WAYLAND_SCANNER_BIN)
            message(STATUS "Found wayland-scanner: ${WAYLAND_SCANNER_BIN}")
        else()
            message(WARNING "wayland-scanner not found. Wayland backend will be disabled.")
            set(WAYLAND_BACKEND_ENABLED FALSE)
        endif()
    endif()
    
    # Download wlr-screencopy protocol if not present
    if(WAYLAND_BACKEND_ENABLED)
        set(PROTOCOL_DIR "${CMAKE_CURRENT_BINARY_DIR}/protocols")
        file(MAKE_DIRECTORY ${PROTOCOL_DIR})
        
        set(WLR_SCREENCOPY_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-screencopy-unstable-v1.xml")
        
        # Check if protocol exists locally
        if(NOT EXISTS ${WLR_SCREENCOPY_XML})
            message(STATUS "Downloading wlr-screencopy protocol...")
            
            # Download from wlroots GitHub
            set(PROTOCOL_URL "https://raw.githubusercontent.com/swaywm/wlr-protocols/master/unstable/wlr-screencopy-unstable-v1.xml")
            file(DOWNLOAD 
                ${PROTOCOL_URL}
                ${WLR_SCREENCOPY_XML}
                STATUS DOWNLOAD_STATUS
                TIMEOUT 10
            )
            
            list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
            if(NOT STATUS_CODE EQUAL 0)
                message(WARNING "Failed to download wlr-screencopy protocol. Wayland backend will be disabled.")
                message(WARNING "You can manually place wlr-screencopy-unstable-v1.xml in cpp/Capture/protocols/")
                set(WAYLAND_BACKEND_ENABLED FALSE)
            else()
                message(STATUS "Successfully downloaded wlr-screencopy protocol")
            endif()
        else()
            message(STATUS "Found wlr-screencopy protocol in project")
        endif()
        
        # Generate protocol bindings if we have the XML
        if(WAYLAND_BACKEND_ENABLED AND EXISTS ${WLR_SCREENCOPY_XML})
            set(WLR_SCREENCOPY_CLIENT_H "${PROTOCOL_DIR}/wlr-screencopy-unstable-v1-client-protocol.h")
            set(WLR_SCREENCOPY_CODE "${PROTOCOL_DIR}/wlr-screencopy-unstable-v1-protocol.c")
            
            # Generate client header
            add_custom_command(
                OUTPUT ${WLR_SCREENCOPY_CLIENT_H}
                COMMAND ${WAYLAND_SCANNER_BIN} client-header ${WLR_SCREENCOPY_XML} ${WLR_SCREENCOPY_CLIENT_H}
                DEPENDS ${WLR_SCREENCOPY_XML}
                COMMENT "Generating wlr-screencopy client header"
            )
            
            # Generate protocol code
            add_custom_command(
                OUTPUT ${WLR_SCREENCOPY_CODE}
                COMMAND ${WAYLAND_SCANNER_BIN} private-code ${WLR_SCREENCOPY_XML} ${WLR_SCREENCOPY_CODE}
                DEPENDS ${WLR_SCREENCOPY_XML}
                COMMENT "Generating wlr-screencopy protocol code"
            )
            
            set(WAYLAND_PROTOCOL_SOURCES ${WLR_SCREENCOPY_CODE})
        else()
            set(WAYLAND_BACKEND_ENABLED FALSE)
        endif()
    endif()
endif()

# ============================================================================
# Build Configuration
# ============================================================================

# Start with base sources
set(CAPTURE_SOURCES
    src/i3ipc.cpp
    src/CaptureManager.cpp
)

# Add X11 backend if available
if(X11_BACKEND_ENABLED)
    list(APPEND CAPTURE_SOURCES src/backends/XWaylandBackend.cpp)
    message(STATUS "X11/XWayland backend: ENABLED")
else()
    message(STATUS "X11/XWayland backend: DISABLED")
endif()

# Add Wayland backend if available
if(WAYLAND_BACKEND_ENABLED)
    list(APPEND CAPTURE_SOURCES 
        src/backends/WaylandBackend.cpp
        ${WAYLAND_PROTOCOL_SOURCES}
    )
    message(STATUS "Wayland backend: ENABLED")
else()
    message(STATUS "Wayland backend: DISABLED")
endif()

# Create the library
add_library(Capture STATIC ${CAPTURE_SOURCES})

# Include directories
target_include_directories(Capture PUBLIC include)
target_include_directories(Capture PUBLIC ../IMGBuffer/include)

if(WAYLAND_BACKEND_ENABLED)
    target_include_directories(Capture PRIVATE ${PROTOCOL_DIR})
endif()

# Link base dependencies
target_link_libraries(Capture PUBLIC IMGBuffer)

# Configure X11 backend
if(X11_BACKEND_ENABLED)
    target_compile_definitions(Capture PUBLIC X11_BACKEND_ENABLED)
    target_link_libraries(Capture PRIVATE ${X11_LIBRARIES})
    target_include_directories(Capture PRIVATE ${X11_INCLUDE_DIRS})
endif()

# Configure Wayland backend
if(WAYLAND_BACKEND_ENABLED)
    target_compile_definitions(Capture PUBLIC WAYLAND_BACKEND_ENABLED)
    target_link_libraries(Capture PRIVATE ${WAYLAND_LIBRARIES})
    target_include_directories(Capture PRIVATE ${WAYLAND_INCLUDE_DIRS})
endif()

# ============================================================================
# JSON Dependency (always needed for i3ipc)
# ============================================================================
include(FetchContent)
FetchContent_Declare(
    json 
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)
target_link_libraries(Capture PUBLIC nlohmann_json::nlohmann_json)

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "=== Capture Library Configuration ===")
message(STATUS "X11/XWayland Backend: ${X11_BACKEND_ENABLED}")
message(STATUS "Wayland Backend:      ${WAYLAND_BACKEND_ENABLED}")
message(STATUS "=====================================")
message(STATUS "")