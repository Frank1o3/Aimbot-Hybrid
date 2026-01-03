add_library(IMGBuffer STATIC
    src/buffer.cpp
)

target_include_directories(IMGBuffer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(IMGBuffer PUBLIC cxx_std_17)
