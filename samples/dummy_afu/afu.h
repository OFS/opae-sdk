/*
 * afu.h
 */
#pragma once
#include <regex.h>
#include <string>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>


#define PARSE_MATCH_INT(_p, _m, _v, _b)                            \
	do {                                                             \
		errno = 0;                                                     \
		_v = std::strtoul(_p + _m.rm_so, NULL, _b);                    \
		if (errno) {                                                   \
			throw std::runtime_error("error parsing number");            \
		}                                                              \
	} while (0)

using namespace opae::fpga::types;


const char *sbdf_pattern = "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])";
const char *bdf_pattern = "([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])";

enum {
  MATCHES_SIZE = 6
};
bool parse_bdf(const char* bdf, uint16_t &bus, uint16_t &device, uint8_t &function)
{
  regex_t re1, re2;
  regmatch_t matches[MATCHES_SIZE];

  int reg_res = regcomp(&re1, sbdf_pattern, REG_EXTENDED | REG_ICASE);
  if (reg_res) {
    throw std::runtime_error("could not compile regex");
  }
  reg_res = regcomp(&re2, bdf_pattern, REG_EXTENDED | REG_ICASE);
  if (reg_res) {
    throw std::runtime_error("could not compile regex");
  }

  uint32_t segment = 0;
  reg_res = regexec(&re1, bdf, MATCHES_SIZE, matches, 0);
  if (reg_res) {
    reg_res = regexec(&re2, bdf, MATCHES_SIZE, matches, 0);
  } else {
    PARSE_MATCH_INT(bdf, matches[1], segment, 16);
	  PARSE_MATCH_INT(bdf, matches[2], bus, 16);
	  PARSE_MATCH_INT(bdf, matches[3], device, 16);
	  PARSE_MATCH_INT(bdf, matches[4], function, 10);
    return true;
  }
  (void)segment;
  if (reg_res) {
    char err[128];
    regerror(reg_res, &re2, err, 128);
    throw std::runtime_error(err);
  }
  //PARSE_MATCH_INT(buffer, matches[1], device->segment, 16, out);
	PARSE_MATCH_INT(bdf, matches[1], bus, 16);
	PARSE_MATCH_INT(bdf, matches[2], device, 16);
	PARSE_MATCH_INT(bdf, matches[3], function, 10);
  return true;

}

handle::ptr_t open_accelerator(const char *afu_id,
                               const char *bdf=nullptr)
{

  auto filter = properties::get();
  filter->type = FPGA_ACCELERATOR;
  filter->guid.parse(afu_id);
  if (bdf) {
    uint16_t bus, device;
    uint8_t function;
    if (parse_bdf(bdf, bus, device, function)){
      filter->bus = bus;
      filter->device = device;
      filter->function = function;
    }

    
  }

  auto tokens = token::enumerate({filter});
  if (tokens.size() < 1) {
    return nullptr;
  }
  if (tokens.size() > 1) {
    std::cerr << "more than one accelerator found matching filter\n";
  }
  return handle::open(tokens[0], 0);
}
