cmake_minimum_required(VERSION 3.15)
project(xwayland_capture LANGUAGES CXX)

# --------------------
# Toolchain
# --------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# --------------------
# Python / pybind11
# --------------------
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    include(FetchContent)
    set(PYBIND11_TEST OFF CACHE BOOL "" FORCE)
    set(PYBIND11_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.12.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# --------------------
# Wayland
# --------------------
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
endif()

# --------------------
# Module
# --------------------
pybind11_add_module(xwayland_capture
    bindings/module.cpp
    bindings/capture_bind.cpp
    src/capture.cpp
)

target_include_directories(xwayland_capture PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(UNIX AND NOT APPLE)
    target_link_libraries(xwayland_capture PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    target_include_directories(xwayland_capture PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_compile_options(xwayland_capture PRIVATE ${WAYLAND_CLIENT_CFLAGS_OTHER})
endif()

# --------------------
# Output location
# --------------------
# Build dir = build/
# Result = build/libs/xwayland_capture.so
set_target_properties(xwayland_capture PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs
)

# --------------------
# Stub Generation (.pyi)
# --------------------
find_program(PYBIND11_STUBGEN pybind11-stubgen)

if(PYBIND11_STUBGEN)
    add_custom_command(
        TARGET xwayland_capture POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_BINARY_DIR}/libs"
                ${Python3_EXECUTABLE} -m pybind11_stubgen xwayland_capture
                --output-dir ${CMAKE_BINARY_DIR}/libs
        COMMENT "Generating python stubs for xwayland_capture..."
    )
else()
    message(WARNING "pybind11-stubgen not found. IDE autocomplete will not work.")
endif()

message(STATUS "Module name : xwayland_capture")
message(STATUS "Python      : ${Python3_VERSION}")
message(STATUS "Build type  : ${CMAKE_BUILD_TYPE}")
