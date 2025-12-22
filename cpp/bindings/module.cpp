#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_capture(py::module_&);

PYBIND11_MODULE(xwayland_capture, m) {
    m.doc() = "XWayland capture backend";
    bind_capture(m);
}
