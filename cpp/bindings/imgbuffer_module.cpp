#include <pybind11/pybind11.h>
namespace py = pybind11;
void bind_imgbuffer(py::module_& m);

// Internal name must match build.py extension name
PYBIND11_MODULE(imgbuffer, m) {
    bind_imgbuffer(m);
}