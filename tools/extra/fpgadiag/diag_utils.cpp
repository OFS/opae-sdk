#include "diag_utils.h"

using namespace opae::fpga::types;

namespace intel
{
namespace fpga
{

properties::ptr_t get_properties(intel::utils::option_map::ptr_t opts, fpga_objtype otype)
{
    using intel::utils::option;
    properties::ptr_t props =
	    properties::get(otype);

    uint8_t bus = 0;
    option::ptr_t opt = opts->find("bus");
    if (opt && opt->is_set() && opts->get_value<uint8_t>("bus", bus)) {
        props->bus = bus;
    }
    uint8_t device = 0;
    opt = opts->find("device");
    if (opt && opt->is_set() && opts->get_value<uint8_t>("device", device)) {
        props->device = device;
    }
    uint8_t function = 0;
    opt = opts->find("function");
    if (opt && opt->is_set() && opts->get_value<uint8_t>("function", function)) {
        props->function = function;
    }
    uint8_t socket_id = 0;
    opt = opts->find("socket-id");
    if (opt && opt->is_set() && opts->get_value<uint8_t>("socket-id", socket_id)) {
        props->socket_id = socket_id;
    }
    opt = opts->find("guid");
    if (opt) {
      props->guid.parse(opt->value<std::string>().c_str());
    }
    return props;
}

token::ptr_t get_parent_token(handle::ptr_t h)
{
    auto props = properties::get(h);

    auto tokens = token::enumerate({properties::get(props->parent)});
    if (!tokens.empty())
    {
        return tokens[0];
    }
    return token::ptr_t();
}

uint64_t umsg_num(handle::ptr_t h)
{
    uint64_t num = 0;
    fpga_result res = fpgaGetNumUmsg(*h, &num);
    if (res == FPGA_OK)
       return num;
    return 0;
}

bool umsg_set_mask(handle::ptr_t h, uint64_t mask)
{
    return FPGA_OK == fpgaSetUmsgAttributes(*h, mask);
}

uint64_t * umsg_get_ptr(handle::ptr_t h)
{
    uint64_t *p = NULL;
    fpga_result res = fpgaGetUmsgPtr(*h, &p);
    if (res == FPGA_OK)
        return p;
    return NULL;
}

} // end of namespace fpga
} // end of namespace intel
