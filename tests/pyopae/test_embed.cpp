
#include <Python.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(testembed, m) {
  m.def("zero", []() { return 0;});
}

int main(int argc, char *argv[]) {
  return 0;
}
