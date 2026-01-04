// capture_bind.cpp
#include <pybind11/pybind11.h>
#include <CaptureManager.hpp>

namespace py = pybind11;

void bind_capture(py::module_ &m) {
    // Enum for backend types
    py::enum_<Capture::BackendType>(m, "BackendType", R"doc(
        Specifies which capture backend to use.
    )doc")
        .value("Auto", Capture::BackendType::Auto, "Automatically detect the best backend")
        .value("XWayland", Capture::BackendType::XWayland, "Use X11 (works with both pure X11 and XWayland)")
        .value("Wayland", Capture::BackendType::Wayland, "Use native Wayland")
        .export_values();

    // CaptureManager class
    py::class_<Capture::CaptureManager>(m, "CaptureManager", R"doc(
        High-performance screen capture manager with triple buffering.
        
        Supports multiple backends: XWayland, Wayland, and X11.
    )doc")
        .def(py::init<const std::string&>(), 
             py::arg("window_name"),
             "Create a CaptureManager with auto-detected backend")
        
        .def(py::init<const std::string&, Capture::BackendType>(), 
             py::arg("window_name"), 
             py::arg("backend"),
             "Create a CaptureManager with specified backend")
        
        .def("init", &Capture::CaptureManager::init,
             "Initialize the capture system and find the target window")
        
        .def("start", &Capture::CaptureManager::start,
             "Start the background capture thread")
        
        .def("stop", &Capture::CaptureManager::stop,
             "Stop the background capture thread")
        
        .def("set_backend", &Capture::CaptureManager::setBackend,
             py::arg("backend"),
             "Change the backend (must call before init or after stop)")
        
        .def("get_backend", &Capture::CaptureManager::getBackend,
             "Get the current backend type")
        
        .def("get_backend_name", &Capture::CaptureManager::getBackendName,
             "Get the current backend name as a string")
        
        .def_property_readonly("frame_count", &Capture::CaptureManager::getFrameCount,
             "Number of frames captured since start")
        
        .def_property_readonly("buffer", &Capture::CaptureManager::getActiveBuffer,
             py::return_value_policy::reference_internal,
             "Get the most recent captured frame buffer");
}