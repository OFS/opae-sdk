#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <editline/readline.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <regex.h>
#include <string>
#include <list>
#include <sys/stat.h>
#include "vfiocpp.h"
#include "pymain.h"

namespace py = pybind11;
using namespace vfio;

device::ptr_t the_device(0);
region::ptr_t the_region(0);

const char *program = "opae-io";
const int major = 0;
const int minor = 2;
const int patch = 0;

py::tuple version()
{
  return py::make_tuple(program, major, minor, patch);
}

void open_device(const std::string &pci_address)
{
  if (the_device) {
    the_device->close();
    the_device.reset();
  }
  if (the_region)
    the_region.reset();
  if (!pci_address.empty())
    the_device = device::open_pciaddress(pci_address);
  auto g = py::globals();
  g["the_device"] = the_device;
  g["the_region"] = the_region;
}

void open_region(uint32_t region_num)
{
  auto g = py::globals();
  if (the_region) {
    the_region.reset();
    g["the_region"] = the_region;
  }
  if (the_device) {
    for (auto r : the_device->regions()) {
      if (r->index() == region_num) {
        the_region = r;
        g["the_region"] = the_region;
        break;
      }
    }
  }
}

uint64_t read_csr(uint64_t offset)
{
  if (the_device && the_region) {
    return the_region->read64(offset);
  }
  return 0;
}

void peek(uint64_t offset)
{
  if (the_region) {
    auto value = read_csr(offset);
    std::cout << "0x" << std::hex << value << "\n";
  } else {
    std::cerr << "no device or region open\n";
  }
}

void write_csr(uint64_t offset, uint64_t value)
{
  if (the_device && the_region) {
    the_region->write64(offset, value);
  }
}

void poke(uint64_t offset, uint64_t value)
{
  if (the_region) {
    write_csr(offset, value);
  } else {
    std::cerr << "no device or region open\n";
  }
}

system_buffer::ptr_t allocate_buffer(size_t sz)
{
  return system_buffer::allocate(sz);
}

py::module import_builtins()
{
      auto sys = py::module::import("sys");
      auto version_info = sys.attr("version_info").cast<py::tuple>();
      if (version_info[0].cast<int>() == 3)
        return py::module::import("builtins");
      else
        return py::module::import("__builtin__");
}

class opae_io_cli {
  public:
    opae_io_cli()
    : interactive_(true)
    , return_code_(0)
    , device_(nullptr)
    , region_(nullptr)
    {
    }

    void update_device(device::ptr_t device, region::ptr_t region)
    {
      device_ = device;
      region_ = region;
      the_device = device;
      the_region = region;
      auto builtins = import_builtins();
      builtins.attr("the_device") = the_device;
      builtins.attr("the_region") = the_region;
    }

    device::ptr_t get_device()
    {
      return device_;
    }

    region::ptr_t get_region()
    {
      return region_;
    }

    bool is_interactive()
    {
      return interactive_;
    }

    int return_code()
    {
      return return_code_;
    }

    void set_return_code(int code)
    {
      interactive_ = false;
      return_code_ = code;
    }

  private:
    bool interactive_;
    int return_code_;
    device::ptr_t device_;
    region::ptr_t region_;

};

void command_line_help(void);

PYBIND11_EMBEDDED_MODULE(opae_io, m) {
  m.def("read_csr", &read_csr);
  m.def("peek", &peek);
  m.def("write_csr", &write_csr);
  m.def("poke", &poke);
  m.def("device", &open_device,
        "Open a device given a pci address",
        py::arg("pci_address") = "");
  m.def("region", &open_region);
  m.def("allocate_buffer", &allocate_buffer);
  m.def("version", &version);
  py::class_<opae_io_cli, std::shared_ptr<opae_io_cli>> pycli(m, "cli", "");
  pycli.def(py::init<>())
       .def("update_device", &opae_io_cli::update_device)
       .def("return_code", &opae_io_cli::set_return_code);
}

char * prompt(void)
{
  static char pstr[256];
  std::stringstream ss;

  if (!the_device) {
    ss << "opae-io>> ";
  } else {
    ss << the_device->pci_address();
    if (the_region)
      ss << "[" << the_region->index() << "]";
    ss << ">> ";
  }

  return strcpy(pstr, ss.str().c_str());
}

void help(void)
{
  std::cout << "use help(topic) on the following topics:" << std::endl
            << "  ls" << std::endl
            << "  peek" << std::endl
            << "  poke" << std::endl
            << "  read_csr" << std::endl
            << "  write_csr" << std::endl
            << "  device" << std::endl
            << "  region" << std::endl
            << "  the_device" << std::endl
            << "  the_region" << std::endl;
}

void add_keys(py::dict items, std::vector<std::string> &keys) {
    for (auto kv: items) {
      std::string key = py::str(kv.first);
      keys.push_back(key);
      if (key == "__builtins__") {
        add_keys(kv.second.attr("__dict__"), keys);
      }
    }
}

char *pygenerator(const char *text, int state) {
  if (std::string(text).empty()) return nullptr;
  std::string prefix = "";
  std::string suffix = "";
  auto str_text = std::string(text);

  static int index = 0;
  static std::vector<std::string> keys;
  static std::vector<std::string> names(0);
  static bool device_context = false;
  if (!state) {
    names.clear();
    device_context = false;
    py::dict scope = py::globals();
    if (str_text.find("device(\"") != std::string::npos) {
      auto lsfpga = scope["lsfpga"];
      for (auto it : lsfpga()) {
        py::tuple t = it.cast<py::tuple>();
        keys.push_back(py::str(t[0]));
      }
      device_context = true;
    } else {
      py::module pymain = py::module::import("__main__");
      auto builtins = scope["__builtins__"];
      auto dir = builtins.attr("dir");
      auto pos = str_text.find_first_of(".");
      py::object obj;
      while (pos != std::string::npos) {
        auto lhs = str_text.substr(0, pos);
        if (names.empty()) {
          obj = scope[lhs.c_str()];
        } else {
          obj = obj.attr(lhs.c_str());
        }
        names.push_back(lhs);
        str_text = str_text.substr(pos+1);
        pos = str_text.find_first_of(".");
      }

      if (names.empty()) {
        add_keys(py::globals(), keys);
        keys.push_back("import");
      } else {
        for (auto o : dir(obj)) {
          std::string k = py::str(o);
          if (k.find_first_of("__") != 0)
            keys.push_back(py::str(o));
        }
      }
    }
    index = 0;
  }

  if (device_context) {
    auto pos = str_text.find_last_of("(\"");
    if (pos != std::string::npos) {
      pos += 1;
      prefix = str_text.substr(0, pos);
      str_text = pos == str_text.size() ? "" : str_text.substr(pos);
      if (keys.size() == 1)
        suffix = "\")";
    }
  } else {
    str_text = std::string(text);
    auto pos = str_text.find_last_of(".");
    if (pos != std::string::npos) {
      if (pos + 1 == str_text.size()) {
        str_text = "";
      } else {
        prefix = str_text.substr(0, pos+1);
        str_text = str_text.substr(pos+1);
      }
    }
  }
  
  for (size_t i = index; i < keys.size(); ++i) {
    auto key = keys[i];
    if (str_text == "" ||
        key.find(str_text) == 0 ) {
      index = i+1;
      return strdup((prefix + key).c_str());
    }
    auto pos = str_text.find_last_of("device(\"");
    if (pos != std::string::npos && str_text.substr(pos) == key) {
      index = i+1;
      return strdup((prefix + key + suffix).c_str());
    }
  }
  keys.clear();
  return nullptr;
}

char **custom_completer(const char *text, int start, int end)
{
  return completion_matches((char *)text, pygenerator);
}

char * readline_init(void);
void readline_destroy(char * );


py::list args_to_list(int argc, char *argv[])
{
  py::list pyargs;
  for (int i = 0; i < argc; ++i) pyargs.append(argv[i]);
  return pyargs;

}

bool is_file(const std::string &path)
{
  struct stat stbuf;
  return !stat(path.c_str(), &stbuf);
}


int main(int argc, char *argv[])
{
  int res = 0;
  py::scoped_interpreter guard{}; // start the interpreter and keep it alive
  auto cli = std::make_shared<opae_io_cli>();
  auto globals = py::globals();
  auto opae_io = py::module::import("opae_io");
  auto libvfio = py::module::import("libvfio");
  py::module builtins = import_builtins();
  globals["builtins"] = builtins;

  builtins.attr("the_device") = the_device;
  builtins.attr("the_region") = the_region;
  builtins.attr("peek") = opae_io.attr("peek");
  builtins.attr("read_csr") = opae_io.attr("read_csr");
  builtins.attr("write_csr") = opae_io.attr("write_csr");
  builtins.attr("poke") = opae_io.attr("poke");
  builtins.attr("device") = opae_io.attr("device");
  builtins.attr("region") = opae_io.attr("region");
  builtins.attr("allocate_buffer") = opae_io.attr("allocate_buffer");
  builtins.attr("version") = opae_io.attr("version");
  builtins.attr("cli") = cli;

  auto pysys = py::module::import("sys");
  auto pyargs = args_to_list(argc, argv);
  pysys.attr("argv") = pyargs;
  py::exec(pymain, globals);
  if (!cli->is_interactive()) {
    return cli->return_code();
  }

  //builtins.attr("the_device") = the_device;
  //builtins.attr("the_region") = the_region;


  char *history = readline_init();
  container::instance(); 
  while (true) {
    char *l = readline(prompt());

    if (!l) {
      std::cout << "\n";
      break;
    }
 
    std::string line(l);
  
    if (line.back() == '\n') {
      line.pop_back();
    }
  
    if (line == "q" ||
        line == "quit" || line == "exit" ||
        line == "quit()" || line == "exit()") {
      break;
    }
  
    if (line == "help") {
      help();
    } else if (line.length()) {
      try {
        py::exec(line, globals);
      } catch(py::error_already_set &pyerr) {
        if (!pyerr.matches(PyExc_SystemExit)) {
          std::cerr << pyerr.what() << std::endl;
        } else {
          break;
        }
      }
    
      add_history(line.c_str());
    }
  }

  readline_destroy(history);

  return res;
}

bool regex_matches(std::string haystack, std::string pattern)
{
  regex_t re;
  regmatch_t matches[3] = { {0} };
  regcomp(&re, pattern.c_str(), REG_EXTENDED|REG_ICASE);
  bool match = regexec(&re,
                       haystack.c_str(),
                       2,
                       matches,
                       0) == 0;
  if (matches[0].rm_so != 0 ||
      matches[0].rm_eo != static_cast<int>(haystack.length())) {
    match = false;
  }
  regfree(&re);
  return match;
}

bool is_pci_address(std::string &pci_address)
{
  std::string sbdf("([0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\\.[0-7])");
  if (regex_matches(pci_address, sbdf)) {
    return true;
  }

  std::string bdf("([0-9a-fA-F]{2}:[0-9a-fA-F]{2}\\.[0-7])");
  if (regex_matches(pci_address, bdf)) {
    pci_address = "0000:" + pci_address;
    return true;
  }

  return false;
}

bool is_decimal(std::string number)
{
  std::string pattern("[0-9]+");
  return regex_matches(number, pattern);
}

bool is_hex(std::string number)
{
  std::string pattern("0x[0-9a-fA-F]+");
  return regex_matches(number, pattern);
}

//opae_io_cli::opae_io_cli(int argc, char *argv[])
//{
//  int i;
//  int state;
//  struct stat stbuf;
//
//  flags = (argc < 2) ? 0 : FLAG_INVALID;
//
//#define START            0
//#define GOT_PCI_ADDRESS  1
//#define GOT_REGION       2
//#define GOT_INIT         3
//#define GOT_RELEASE      4
//#define GOT_PEEK_CMD     5
//#define GOT_POKE_CMD     6
//#define GOT_POKE_OFFSET  7
//#define GOT_INIT_ADDRESS 8
//#define GOT_SCRIPT       9
//#define STOP            99
//
//  state = START;
//  i = 1;
//  while (state != STOP && i < argc) {
//
//    std::string input(argv[i]);
//
//    switch (state) {
//      case START : { // opae-io
//        if (is_pci_address(input)) {
//          flags |= FLAG_OPEN_DEVICE;
//          pci_address = input;
//          state = GOT_PCI_ADDRESS;
//          flags &= ~FLAG_INVALID;
//        } else if (input == "-h" || input == "--help") {
//          flags |= FLAG_SHOW_HELP;
//          flags &= ~FLAG_INVALID;
//        } else if (input == "-v" || input == "--version") {
//          flags |= FLAG_SHOW_VERSION;
//          flags &= ~FLAG_INVALID;
//        } else if (input == "init") {
//          flags |= FLAG_EXEC_CMD;
//          command = "opae_io_init(";
//          state = GOT_INIT;
//        } else if (input == "release") {
//          flags |= FLAG_EXEC_CMD;
//          command = "opae_io_release(";
//          state = GOT_RELEASE;
//        } else if (input == "ls") {
//          flags &= ~FLAG_INVALID;
//          flags |= FLAG_EXEC_CMD;
//          state = STOP;
//          command = "ls()";
//        } else if (!stat(input.c_str(), &stbuf)) {
//          flags |= FLAG_RUN_SCRIPT;
//          script = input;
//          state = GOT_SCRIPT;
//          flags &= ~FLAG_INVALID;
//        }
//      } break;
//
//      case GOT_SCRIPT : { // opae-io script
//        script_args.push_back(input);
//        // Don't change state.
//        // Continue collecting args until we run out.
//      } break;
//
//      case GOT_PCI_ADDRESS : { // opae-io 0000:00:00.0
//        if (is_decimal(input) || is_hex(input)) {
//          flags |= FLAG_OPEN_REGION;
//          region_num = input;
//          state = GOT_REGION;
//        } else if (input == "-h" || input == "--help") {
//          flags |= FLAG_SHOW_HELP;
//        } else if (input == "-v" || input == "--version") {
//          flags |= FLAG_SHOW_VERSION;
//        }
//      } break;
//
//      case GOT_REGION : { // opae-io 0000:00:00.0 2
//        if (input == "-h" || input == "--help") {
//          flags |= FLAG_SHOW_HELP;
//          flags &= ~FLAG_INVALID;
//        } else if (input == "-v" || input == "--version") {
//          flags |= FLAG_SHOW_VERSION;
//          flags &= ~FLAG_INVALID;
//        } else if (input == "peek") {
//          flags |= FLAG_EXEC_CMD|FLAG_INVALID;
//          command = "peek(";
//          state = GOT_PEEK_CMD;
//        } else if (input == "poke") {
//          flags |= FLAG_EXEC_CMD|FLAG_INVALID;
//          command = "poke(";
//          state = GOT_POKE_CMD;
//        } else if (!stat(input.c_str(), &stbuf)) {
//          flags |= FLAG_RUN_SCRIPT;
//          flags &= ~FLAG_INVALID;
//          script = input;
//          state = GOT_SCRIPT;
//        }
//      } break;
//
//      case GOT_INIT : { // opae-io init
//        if (is_pci_address(input)) {
//          command += "'" + input + "', ";
//          state = GOT_INIT_ADDRESS;
//        } else {
//          flags |= FLAG_INVALID;
//          state = STOP;
//        }
//      } break;
//
//      case GOT_INIT_ADDRESS : { // opae-io init 0000:00:00.0
//        command += "'" + input + "')";
//        state = STOP;
//        flags &= ~FLAG_INVALID;
//      } break;
//
//      case GOT_RELEASE : { // opae-io release
//        if (is_pci_address(input)) {
//          command += "'" + input + "')";
//          flags &= ~FLAG_INVALID;
//        } else {
//          flags |= FLAG_INVALID;
//        }
//        state = STOP;
//      } break;
//
//      case GOT_PEEK_CMD : { // opae-io 0000:00:00.0 0 peek
//        if (is_decimal(input) || is_hex(input)) {
//          command += input + ")";
//          flags &= ~FLAG_INVALID;
//        } else {
//          flags |= FLAG_INVALID;
//        }
//        state = STOP;
//      } break;
//
//      case GOT_POKE_CMD : { // opae-io 0000:00:00.0 0 poke
//        if (is_decimal(input) || is_hex(input)) {
//          command += input + ", ";
//          state = GOT_POKE_OFFSET;
//        } else {
//          flags |= FLAG_INVALID;
//          state = STOP;
//        } 
//      } break;
//
//      case GOT_POKE_OFFSET : { // opae-io 0000:00:00.0 0 poke 0x28
//        if (is_decimal(input) || is_hex(input)) {
//          command += input + ")";
//          flags &= ~FLAG_INVALID;
//        } else {
//          flags |= FLAG_INVALID;
//        } 
//        state = STOP;
//      } break;
//    }
//
//    ++i;
//  }
//}

char * readline_init(void)
{
  rl_initialize();
  rl_parse_and_bind("bind ^I rl_complete");
  //rl_parse_and_bind("set editing-mode vi");
  rl_set_prompt(prompt());
  //rl_completer_quote_characters = strdup("\"'");
  rl_basic_word_break_characters = strdup("\t\n ");
  rl_completer_word_break_characters = strdup("\t\n ");
  rl_attempted_completion_function = custom_completer;
  rl_completion_append_character = '\0';
  using_history();
  static char hfile[256] = { "~/.opae_history" };
  char *hptr = hfile;
  hptr = tilde_expand(hptr);
  struct stat hst;
  if (!stat(hptr, &hst)) {
    if (read_history(hptr)) {
      std::cerr << "error reading history\n"; 
    }
  }
  return hptr;
}

void readline_destroy(char *history)
{
  if (history) {
    if (write_history(history)) {
      std::cerr << "error writing history" << std::endl;
    }
    free(history);
  }
}

void command_line_help(void)
{
  std::cout << "opae-io - peek and poke FPGA CSRs" << std::endl
            << std::endl
            << "\topae-io" << std::endl
            << "\topae-io -v | --version" << std::endl
            << "\topae-io -h | --help" << std::endl
            << "\topae-io ls" << std::endl
            << "\topae-io init <PCI_ADDRESS> <USER>[:<GROUP>]" << std::endl
            << "\topae-io release <PCI_ADDRESS>" << std::endl
            << "\topae-io <PCI_ADDRESS>" << std::endl
            << "\topae-io <PCI_ADDRESS> <REGION_NUMBER>" << std::endl
            << "\topae-io <PCI_ADDRESS> <REGION_NUMBER> peek <OFFSET>" << std::endl
            << "\topae-io <PCI_ADDRESS> <REGION_NUMBER> poke <OFFSET> <VALUE>" << std::endl
            << "\topae-io <SCRIPT> <ARG1> <ARG2> ... <ARGN>" << std::endl
            << "\topae-io <PCI_ADDRESS> <REGION_NUMBER> <SCRIPT> <ARG1> <ARG2> ... <ARGN>" << std::endl
            << std::endl;

  std::cout << "EXAMPLES" << std::endl
            << std::endl;

  std::cout << "\tEnumerating FPGA's:" << std::endl
            << std::endl
            << "\t\t$ opae-io ls" << std::endl
            << std::endl;

  std::cout << "\tInitiating a session:" << std::endl
            << std::endl
            << "\t\t$ sudo opae-io init 0000:00:00.0 lab:lab" << std::endl
            << std::endl;

  std::cout << "\tTerminating a session:" << std::endl
            << std::endl
            << "\t\t$ sudo opae-io release 0000:00:00.0" << std::endl
            << std::endl;

  std::cout << "\tEntering an interactive Python environment:" << std::endl
            << std::endl
            << "\t\t$ opae-io 0000:00:00.0 0" << std::endl
            << std::endl;

  std::cout << "\tPeek & Poke from the command line:" << std::endl
            << std::endl
            << "\t\t$ opae-io 0000:00:00.0 0 peek 0x28" << std::endl
            << "\t\t$ opae-io 0000:00:00.0 0 poke 0x28 0xbaddecaf" << std::endl
            << std::endl;

  std::cout << "\tExecuting a script:" << std::endl
            << std::endl
            << "\t\t$ opae-io 0000:00:00.0 0 script.py a b c" << std::endl
            << std::endl;
}
