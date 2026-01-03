include(FetchContent)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(Capture STATIC
    src/CaptureManager.cpp
    src/backends/XWaylandBackend.cpp
)

target_include_directories(Capture PUBLIC include)

target_include_directories(Capture PRIVATE src)

find_package(Threads REQUIRED)

target_link_libraries(Capture 
    PUBLIC 
        IMGBuffer 
        Threads::Threads
        nlohmann_json::nlohmann_json
    PRIVATE 
        ${X11_LIBRARIES}
)

target_include_directories(Capture PRIVATE ${X11_INCLUDE_DIRS})