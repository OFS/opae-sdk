// Copyright(c) 2017-2018, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#include <Python.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
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
  pytp.def_static("platforms", &test_platform::platforms)
      .def_static("get", &test_platform::get)
      .def_static("exists", &test_platform::exists)
      .def("is_mock", [](test_platform &p) { return p.mock_sysfs != nullptr; })
      .def_property_readonly("devices",
                             [](test_platform &p) { return p.devices; });
  ;

  py::class_<test_device> pytd(m, "test_device");
  pytd.def_property_readonly("afu_guid",
                             [](test_device &td) { return td.afu_guid; })
      .def_property_readonly("fme_guid",
                             [](test_device &td) { return td.fme_guid; });
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
  } catch (py::error_already_set &ex) {
    test_system::instance()->finalize();
    std::cerr << "error executing: " << testpy << " - " << ex.what() << "\n";
    ex.restore();
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
