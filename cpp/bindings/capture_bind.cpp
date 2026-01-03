#include <pybind11/pybind11.h>
#include <CaptureManager.hpp>

namespace py = pybind11;

void bind_capture(py::module_ &m) {
    py::class_<Capture::CaptureManager>(m, "CaptureManager")
        .def(py::init<const std::string&>(), py::arg("window_name"))
        .def("init", &Capture::CaptureManager::init)
        .def("start", &Capture::CaptureManager::start)
        .def("stop", &Capture::CaptureManager::stop)
        .def_property_readonly("frame_count", &Capture::CaptureManager::getFrameCount)
        
        // This links the two: It returns the Buffer object defined in imgbuffer_bind.cpp
        .def_property_readonly("buffer", &Capture::CaptureManager::getActiveBuffer,
             py::return_value_policy::reference_internal);
}