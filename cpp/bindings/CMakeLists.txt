# --- EXTENSION 1: imgbuffer ---
pybind11_add_module(imgbuffer
    imgbuffer_module.cpp
    imgbuffer_bind.cpp
)
target_include_directories(imgbuffer PRIVATE ${CMAKE_SOURCE_DIR}/IMGBuffer/include)
target_link_libraries(imgbuffer PRIVATE IMGBuffer)

# --- EXTENSION 2: capture ---
pybind11_add_module(capture
    capture_module.cpp
    capture_bind.cpp
)

# Capture needs to see Buffer headers because it returns a Buffer&
target_include_directories(capture PRIVATE 
    ${CMAKE_SOURCE_DIR}/Capture/include
    ${CMAKE_SOURCE_DIR}/IMGBuffer/include
)
# Link everything capture needs
target_link_libraries(capture PRIVATE Capture IMGBuffer nlohmann_json::nlohmann_json)

# Ensure both go to the right place for build.py
set_target_properties(imgbuffer capture PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs
)