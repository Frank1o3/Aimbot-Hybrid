#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_imgbuffer(py::module_&);

PYBIND11_MODULE(imgbuffer, m) {
    m.doc() = "High-performance image buffer backend";

    bind_imgbuffer(m);
}
