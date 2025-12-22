#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "capture.hpp"

namespace py = pybind11;

void bind_capture(py::module_& m) {
    using Capture = capture::xwayland::WaylandCapture;

    py::class_<Capture>(m, "XWaylandCapture")
        .def(py::init<>())
        .def("setup", &Capture::setup)
        .def("capture_frame", &Capture::capture_frame)
        .def("width", &Capture::width)
        .def("height", &Capture::height)
        .def("cleanup", &Capture::cleanup);
}
