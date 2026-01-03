# 1. Define the library
# We add all the source files here. If you add new backends (like Wayland),
# you would add their .cpp files to this list.
add_library(Capture STATIC
    src/CaptureManager.cpp
    src/backends/XWaylandBackend.cpp
)

# 2. Public Headers
# This allows other parts of the project (like 'bindings') to find your .hpp files
target_include_directories(Capture PUBLIC include)

# 3. Private Headers (Internal)
# This lets the backends see their own headers in the src/backends folder
target_include_directories(Capture PRIVATE src)

# 4. Link Dependencies
# - IMGBuffer: The shared memory storage
# - X11: The windowing system
# - Xext: Required specifically for XShm (Shared Memory)
# - Threads: Required for the background capture thread
find_package(Threads REQUIRED)

target_link_libraries(Capture 
    PUBLIC 
        IMGBuffer 
        Threads::Threads
    PRIVATE 
        ${X11_LIBRARIES}
)

# 5. Compiler Flags for X11
# This ensures the compiler knows where the X11 headers are located (/usr/include/X11, etc.)
target_include_directories(Capture PRIVATE ${X11_INCLUDE_DIRS})