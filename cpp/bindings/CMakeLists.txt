pybind11_add_module(imgbuffer
    module.cpp
    imgbuffer_bind.cpp
)

target_link_libraries(imgbuffer PRIVATE IMGBuffer)

# Only set a default if the caller (build.py) didn't provide one
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(imgbuffer PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs
    )
endif()