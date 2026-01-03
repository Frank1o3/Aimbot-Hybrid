#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <buffer.hpp>

namespace py = pybind11;

void bind_imgbuffer(py::module_ &m)
{
    py::class_<IMGBuffer::Buffer>(m, "Buffer", py::buffer_protocol(), R"doc(
        A high-performance RGBA8 image buffer backed by C++.
    )doc")
        .def(py::init<std::size_t, std::size_t>(), 
             py::arg("width"), 
             py::arg("height"),
             "Constructs a new image buffer with given dimensions.")
        
        .def("resize", &IMGBuffer::Buffer::resize, 
             py::arg("width"), 
             py::arg("height"),
             "Resizes the buffer. Warning: This clears existing image data.")

        .def_property_readonly("width", &IMGBuffer::Buffer::width, "The width of the image in pixels.")
        .def_property_readonly("height", &IMGBuffer::Buffer::height, "The height of the image in pixels.")
        .def_property_readonly("stride", &IMGBuffer::Buffer::stride, "The number of bytes per row (Width * 4).")

        // Expose as a NumPy-compatible array without copying memory
        .def_property_readonly("view", [](IMGBuffer::Buffer &b) {
            return py::array_t<std::uint8_t>(
                { b.height(), b.width(), std::size_t(4) },      // Shape: (H, W, C)
                { b.stride(), std::size_t(4), std::size_t(1) }, // Strides in bytes
                b.data(),                                       // Pointer to data
                py::cast(b)                                     // Keep C++ object alive while array exists
            );
        }, "Returns a zero-copy NumPy view of the buffer data.")

        // Enables support for: np.array(buffer_instance) or memoryview(buffer_instance)
        .def_buffer([](IMGBuffer::Buffer &b) -> py::buffer_info {
            return py::buffer_info(
                b.data(),                               /* Pointer to buffer */
                sizeof(std::uint8_t),                  /* Size of one scalar */
                py::format_descriptor<std::uint8_t>::format(), /* Python struct-style format descriptor */
                3,                                      /* Number of dimensions */
                { b.height(), b.width(), std::size_t(4) }, /* Buffer dimensions */
                { 
                    static_cast<py::ssize_t>(b.stride()), 
                    static_cast<py::ssize_t>(4), 
                    static_cast<py::ssize_t>(1) 
                }                                       /* Strides (in bytes) for each index */
            );
        });
}