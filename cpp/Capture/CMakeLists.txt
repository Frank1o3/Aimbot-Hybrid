cmake_policy(SET CMP0135 NEW)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(Capture STATIC
    src/i3ipc.cpp
    src/CaptureManager.cpp
    src/backends/XWaylandBackend.cpp
)

# Tell Capture where to find its own headers and IMGBuffer's headers
target_include_directories(Capture PUBLIC include)
target_include_directories(Capture PUBLIC ../IMGBuffer/include)

# Link the logic
target_link_libraries(Capture PUBLIC IMGBuffer)

# Find X11 for the backend
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11 xext)
target_link_libraries(Capture PRIVATE ${X11_LIBRARIES})
target_include_directories(Capture PRIVATE ${X11_INCLUDE_DIRS})

# Add JSON dependency via FetchContent (as discussed before)
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(Capture PUBLIC nlohmann_json::nlohmann_json)