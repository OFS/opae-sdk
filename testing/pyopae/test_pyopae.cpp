#include <Python.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <fstream>
#include <iostream>
#include "mock/test_system.h"
#include "platform/fpga_hw.h"

namespace py = pybind11;
using namespace opae::testing;

PYBIND11_EMBEDDED_MODULE(mopae, m) {
  m.doc() = "Open Programmable Acceleration Engine";
}

PYBIND11_EMBEDDED_MODULE(mockopae, m) {
  py::class_<test_platform> pytp(m, "test_platform");
  pytp.def_static("platforms", &test_platform::platforms);
  pytp.def_static("get", &test_platform::get);

  py::class_<test_device> pytd(m, "test_device");
  py::class_<test_system> pyts(m, "test_system");

  pyts.def(py::init(&test_system::instance))
      .def("initialize", &test_system::initialize)
      .def("finalize", &test_system::finalize)
      .def("prepare_sysfs", &test_system::prepare_syfs)
      .def("remove_sysfs", &test_system::remove_sysfs);
}

int run_unittest(const char *testpy, py::module pymain) {
  auto globals = py::globals();
  auto mock = py::module::import("mockopae");
  auto unit = py::module::import("unittest");
  auto scope = py::dict(pymain.attr("__dict__"));
  globals["mockopae"] = mock;
  globals["unittest"] = unit;
  try {
    py::eval_file(testpy, scope);
    auto suite = unit.attr("TestLoader")().attr("loadTestsFromModule")(pymain);
    py::dict kwargs;
    kwargs["verbosity"] = 2;
    auto runner = unit.attr("TextTestRunner")(**kwargs);
    auto result = runner.attr("run")(suite);
    return result.attr("wasSuccessful")().cast<bool>() ? 0 : 1;
  } catch (const py::error_already_set &ex) {
    std::cerr << "error executing: " << testpy << " - " << ex.what() << "\n";
  }
  return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
  py::scoped_interpreter guard{};
  auto locals = py::dict();
  auto globals = py::globals();
  auto mopae = py::module::import("mopae");
  auto _opae = py::module::import("_opae");
  mopae.attr("fpga") = _opae;
  globals["opae"] = mopae;
  if (argc > 1) {
    auto pymain = py::module::import("__main__");
    if (argc > 2 && std::string(argv[1]) == "test") {
      return run_unittest(argv[2], pymain);
    }
    py::list pyargv;
    auto sys = py::module::import("sys");
    for (int i = 1; i < argc; ++i) {
      pyargv.append(argv[i]);
    }
    sys.attr("argv") = pyargv;
    try {
      py::eval_file(argv[1], pymain.attr("__dict__"));
    } catch (py::error_already_set &pyerr) {
      if (!pyerr.matches(PyExc_SystemExit)) {
        pyerr.restore();
      }
    }
  }
  return 0;
}
