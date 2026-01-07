# --- EXTENSION 1: imgbuffer ---
pybind11_add_module(IMGBuffer imgbuffer_module.cpp imgbuffer_bind.cpp)
target_link_libraries(IMGBuffer PRIVATE imgbuffer)
target_include_directories(IMGBuffer PRIVATE ${CMAKE_SOURCE_DIR}/imgbuffer/include)
set_target_properties(IMGBuffer PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../src/imgbuffer"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# --- EXTENSION 2: capture ---
pybind11_add_module(Capture capture_module.cpp capture_bind.cpp)
target_include_directories(Capture PRIVATE 
    ${CMAKE_SOURCE_DIR}/capture/include
    ${CMAKE_SOURCE_DIR}/imgbuffer/include
)
target_link_libraries(Capture PRIVATE capture imgbuffer nlohmann_json::nlohmann_json)
set_target_properties(Capture PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../src/capture"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)