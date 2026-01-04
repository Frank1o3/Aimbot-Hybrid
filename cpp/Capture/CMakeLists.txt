# cpp/Capture/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# X11 for XWayland/X11 backends
pkg_check_modules(X11 REQUIRED x11 xext)

# Wayland for Wayland backend
pkg_check_modules(WAYLAND REQUIRED wayland-client)

# Find wayland-scanner for protocol generation
pkg_check_modules(WAYLAND_SCANNER REQUIRED wayland-scanner)
pkg_get_variable(WAYLAND_SCANNER_BIN wayland-scanner wayland_scanner)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

# Generate Wayland protocols
set(PROTOCOL_DIR "${CMAKE_CURRENT_BINARY_DIR}/protocols")
file(MAKE_DIRECTORY ${PROTOCOL_DIR})

# wlr-screencopy-unstable-v1 protocol
set(WLR_SCREENCOPY_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-screencopy-unstable-v1.xml")
set(WLR_SCREENCOPY_CLIENT_H "${PROTOCOL_DIR}/wlr-screencopy-unstable-v1-client-protocol.h")
set(WLR_SCREENCOPY_CODE "${PROTOCOL_DIR}/wlr-screencopy-unstable-v1-protocol.c")

# Check if protocol XML exists, if not provide fallback
if(NOT EXISTS ${WLR_SCREENCOPY_XML})
    message(STATUS "wlr-screencopy protocol not found in project, checking system...")
    # Try common system locations
    set(SYSTEM_PROTOCOL_PATHS
        "/usr/share/wayland-protocols/wlr-screencopy-unstable-v1.xml"
        "/usr/local/share/wayland-protocols/wlr-screencopy-unstable-v1.xml"
    )
    foreach(PATH ${SYSTEM_PROTOCOL_PATHS})
        if(EXISTS ${PATH})
            set(WLR_SCREENCOPY_XML ${PATH})
            message(STATUS "Found wlr-screencopy protocol at: ${PATH}")
            break()
        endif()
    endforeach()
endif()

if(EXISTS ${WLR_SCREENCOPY_XML})
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
    set(WAYLAND_BACKEND_ENABLED TRUE)
else()
    message(WARNING "wlr-screencopy protocol not found. Wayland backend will be disabled.")
    message(WARNING "To enable Wayland support, install wlroots protocols or place wlr-screencopy-unstable-v1.xml in cpp/Capture/protocols/")
    set(WAYLAND_BACKEND_ENABLED FALSE)
    set(WAYLAND_PROTOCOL_SOURCES "")
endif()

# Build the library
if(WAYLAND_BACKEND_ENABLED)
    add_library(Capture STATIC
        src/i3ipc.cpp
        src/CaptureManager.cpp
        src/backends/XWaylandBackend.cpp
        src/backends/WaylandBackend.cpp
        ${WAYLAND_PROTOCOL_SOURCES}
    )
    target_compile_definitions(Capture PUBLIC WAYLAND_BACKEND_ENABLED)
else()
    add_library(Capture STATIC
        src/i3ipc.cpp
        src/CaptureManager.cpp
        src/backends/XWaylandBackend.cpp
    )
endif()

# Include directories
target_include_directories(Capture PUBLIC include)
target_include_directories(Capture PUBLIC ../IMGBuffer/include)
target_include_directories(Capture PRIVATE ${PROTOCOL_DIR})

# Link libraries
target_link_libraries(Capture PUBLIC IMGBuffer)
target_link_libraries(Capture PRIVATE ${X11_LIBRARIES})
target_include_directories(Capture PRIVATE ${X11_INCLUDE_DIRS})

if(WAYLAND_BACKEND_ENABLED)
    target_link_libraries(Capture PRIVATE ${WAYLAND_LIBRARIES})
    target_include_directories(Capture PRIVATE ${WAYLAND_INCLUDE_DIRS})
endif()

# Add JSON dependency
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(Capture PUBLIC nlohmann_json::nlohmann_json)