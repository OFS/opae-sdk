// Copyright(c) 2018, Intel Corporation
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
/*
 * @file argsfilter.h
 *
 * @brief
 */

#ifndef ARGSFILTER_H
#define ARGSFILTER_H
#include <opae/fpga.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Prepare a filter (fpga_properties) from an argument vector.
 *        This sets properties for bus, device, function, or socket-id
 *        if command line options corresponding to those are found in
 *        the argument vector (argv).
 *
 * @param filter The fpga_properties object to modify.
 * @param result An fpga_result value that will be either FPGA_OK if
 *        all OPAE property operations are successful or the result
 *        of the first failing operation.
 * @param argc Pointer to the size of the argument vector.
 * @param argv[] The command line argument vector itself.
 *
 * @return An error code that may be used to exit the program.
 *         This will be EX_OK (0) or EX_SOFTWARE (70). See sysexits for
 *         details on the exit codes.
 *
 * @note This will modify the command line argument vector and count
 *       (argv and argc) if any relevant command line options are found
 *       so that in the end, the resulting argument vector will be missing
 *       those options used for updating the fpga_properties filter.
 */
int set_properties_from_args(fpga_properties filter, fpga_result *result,
			     int *argc, char *argv[]);

#ifdef __cplusplus
}
#endif
#endif /* !ARGSFILTER_H */
