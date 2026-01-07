// Capture_bind.cpp
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include <CaptureManager.hpp>

namespace py = pybind11;

void bind_capture(py::module_ &m)
{
     py::module_::import("IMGBuffer");

     // ====================
     // 2. Bind the enum
     // ====================
     py::enum_<Capture::BackendType>(m, "BackendType", R"doc(
        Specifies which Capture backend to use.
    )doc")
         .value("Auto", Capture::BackendType::Auto, "Automatically detect the best backend")
         .value("XWayland", Capture::BackendType::XWayland, "Use X11/XWayland backend")
         .value("Wayland", Capture::BackendType::Wayland, "Use native Wayland backend")
         .export_values();

     // ====================
     // 3. Bind CaptureManager (now Buffer is known → correct typing)
     // ====================
     py::class_<Capture::CaptureManager>(m, "CaptureManager", R"doc(
        High-performance screen Capture manager with triple buffering.

        Supports X11/XWayland and native Wayland backends.
    )doc")
         .def(py::init<const std::string &>(),
              py::arg("window_name"),
              "Create with auto-detected backend")

         .def(py::init<const std::string &, Capture::BackendType>(),
              py::arg("window_name"), py::arg("backend"),
              "Create with explicitly chosen backend")

         .def("init", &Capture::CaptureManager::init,
              "Find the target window and initialize Capture")

         .def("start", &Capture::CaptureManager::start,
              "Start the background Capture thread")

         .def("stop", &Capture::CaptureManager::stop,
              "Stop the Capture thread")

         .def("set_backend", &Capture::CaptureManager::setBackend,
              py::arg("backend"),
              "Change backend (only before init() or after stop())")

         .def("get_backend", &Capture::CaptureManager::getBackend,
              "Get current backend type")

         .def("get_backend_name", &Capture::CaptureManager::getBackendName,
              "Get current backend as human-readable string")

         .def_property_readonly("frame_count", &Capture::CaptureManager::getFrameCount,
                                "Total frames Captured since start")

         .def_property_readonly("buffer", &Capture::CaptureManager::getActiveBuffer,
                                py::return_value_policy::reference_internal,
                                "Most recent complete frame (type: Buffer)");
}