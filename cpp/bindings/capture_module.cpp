#include <pybind11/pybind11.h>
namespace py = pybind11;
void bind_capture(py::module_& m);

// Internal name must match build.py extension name
PYBIND11_MODULE(capture, m) {
    bind_capture(m);
}