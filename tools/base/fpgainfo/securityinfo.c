// Copyright(c) 2020, Intel Corporation
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

#include <getopt.h>
#include <unistd.h>
#include "fpgainfo.h"
#include "safe_string/safe_string.h"
#include "sysinfo.h"
#include "securityinfo.h"
#include <opae/fpga.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <glob.h>

/*
 * Print help
 */
void security_help(void)
{
    printf("\nPrint PAC security information\n"
           "        fpgainfo security [-h]\n"
           "                -h,--help           Print this help\n"
           "\n");
}

static void read_string(const char *sysfs_path, const char *fname, char *buf,
            int *bytes_read)
{
    char path[SYSFS_PATH_MAX];
    FILE *fd = NULL;

    *bytes_read = 0;

    snprintf_s_ss(path, sizeof(path), "%s/%s", sysfs_path, fname);

    fd = fopen(path, "r");
    if (fd == NULL) {
        FPGA_MSG("open(%s) failed", path);
    } else {
        *bytes_read = fread(buf, 1, SYSFS_PATH_MAX - 1, fd);
        fclose(fd);
        buf[*bytes_read] = '\0';
    }
}

static void build_spi_path(char *buffer, const char * path)
{
    glob_t pglob;
    int gres;
    snprintf_s_ss(buffer, SYSFS_PATH_MAX, "%s/%s", path, SYSFS_SPI0_GLOB);
    gres = glob(buffer, GLOB_NOSORT, NULL, &pglob);
    if (gres){
        globfree(&pglob);
        return;
    }
    snprintf_s_ss(buffer, SYSFS_PATH_MAX, "%sspi_master/%s", 
            pglob.gl_pathv[0], SYSFS_SPI1_GLOB);
    gres = glob(buffer, GLOB_NOSORT, NULL, &pglob);
    if (gres){
        globfree(&pglob);
        return;
    }
    snprintf_s_ss(buffer, SYSFS_PATH_MAX, "%s%s", 
            pglob.gl_pathv[0], SYSFS_SPI1_GLOB);
    gres = glob(buffer, GLOB_NOSORT, NULL, &pglob);
    if (gres){
        globfree(&pglob);
        return;
    }
    strncpy_s(buffer, SYSFS_PATH_MAX, pglob.gl_pathv[0], SYSFS_PATH_MAX);
    globfree(&pglob);
} 

static void print_security_info(fpga_properties props)
{
    const char *sysfs_path = get_sysfs_path(props, FPGA_DEVICE, NULL);
    glob_t pglob;
    int gres;
    char spi_path[SYSFS_PATH_MAX];
    char security_path[SYSFS_PATH_MAX];
    char file_content[SYSFS_PATH_MAX] = {0};
    int bytes_read = 0;
    fpgainfo_print_common("//****** SECURITY ******//", props);

    build_spi_path(spi_path, sysfs_path);
    if(0 == access(spi_path, F_OK)){
        snprintf_s_ss(security_path, sizeof(security_path), "%s%s",
                spi_path, SYSFS_SEC_GLOB);
    }else{
        snprintf_s_ss(security_path, sizeof(security_path), "%s/%s",
                sysfs_path, SYSFS_SEC_GLOB);
    }

    gres = glob(security_path, GLOB_NOSORT, NULL, &pglob);
    if (gres) {
        printf("No security information available\n");
        globfree(&pglob);
        return;
    }
    strncpy_s(security_path, sizeof(security_path), pglob.gl_pathv[0],
            sizeof(security_path));
    globfree(&pglob);
    

    // Print various version strings

    read_string(security_path, SYSFS_SEC_BMC_FWVERS, file_content,
            &bytes_read);

    if (bytes_read) {
        printf("BMC FW Version: ");
        printf("%s", file_content);
    }


    read_string(security_path, SYSFS_SEC_BIP_VER, file_content, &bytes_read);

    if (bytes_read) {
        printf("BIP Version: ");
        printf("%s", file_content);
    }


    read_string(security_path, SYSFS_SEC_FW_VER, file_content, &bytes_read);

    if (bytes_read) {
        printf("TCM FW Version: ");
        printf("%s", file_content);
    }

    read_string(security_path, SYSFS_SEC_CRYPTO_VER, file_content,
            &bytes_read);

    if (bytes_read) {
        printf("Crypto block Version: ");
        printf("%s", file_content);
    }

    // Print programmed hashes
    read_string(security_path, SYSFS_SEC_SR_ROOT, file_content, &bytes_read);
    printf("FIM/SR root entry hash: %s", file_content);

    read_string(security_path, SYSFS_SEC_BMC_ROOT, file_content, &bytes_read);
    printf("BMC root entry hash: %s", file_content);

    read_string(security_path, SYSFS_SEC_PR_ROOT, file_content, &bytes_read);
    printf("PR root entry hash: %s", file_content);

    // Print update counters

    read_string(security_path, SYSFS_SEC_BMC_FLASH_COUNT, file_content,
            &bytes_read);
    printf("BMC flash update counter: %s", file_content);

    read_string(security_path, SYSFS_SEC_QSPI_COUNT, file_content,
            &bytes_read);
    printf("User flash update counter: %s", file_content);

    // Show canceled CSKs

    read_string(security_path, SYSFS_SEC_SR_CANCEL, file_content, &bytes_read);
    printf("FIM/SR CSK IDs canceled: %s",
           strlen(file_content) > 1 ? file_content : "None\n");

    read_string(security_path, SYSFS_SEC_BMC_CANCEL, file_content,
            &bytes_read);
    printf("BMC CSK IDs canceled: %s",
           strlen(file_content) > 1 ? file_content : "None\n");

    read_string(security_path, SYSFS_SEC_PR_CANCEL, file_content, &bytes_read);
    printf("AFU CSK IDs canceled: %s",
           strlen(file_content) > 1 ? file_content : "None\n");

}

fpga_result security_filter(fpga_properties *filter, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    fpga_result res = FPGA_OK;
    res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
    fpgainfo_print_err("setting type to FPGA_DEVICE", res);
    return res;
}

fpga_result security_command(fpga_token *tokens, int num_tokens, int argc,
                 char *argv[])
{
    (void)argc;
    (void)argv;

    fpga_result res = FPGA_OK;
    fpga_properties props;

    optind = 0;
    struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0},
    };

    int getopt_ret;
    int option_index;

    while (-1
           != (getopt_ret = getopt_long(argc, argv, ":h", longopts,
                        &option_index))) {
        const char *tmp_optarg = optarg;

        if ((optarg) && ('=' == *tmp_optarg)) {
            ++tmp_optarg;
        }

        switch (getopt_ret) {
        case 'h': /* help */
            security_help();
            return res;

        case ':': /* missing option argument */
            fprintf(stderr, "Missing option argument\n");
            security_help();
            return FPGA_INVALID_PARAM;

        case '?':
        default: /* invalid option */
            fprintf(stderr, "Invalid cmdline options\n");
            security_help();
            return FPGA_INVALID_PARAM;
        }
    }

    int i = 0;
    for (i = 0; i < num_tokens; ++i) {
        res = fpgaGetProperties(tokens[i], &props);
        ON_FPGAINFO_ERR_GOTO(res, out_destroy,
                     "reading properties from token");
        print_security_info(props);
        fpgaDestroyProperties(&props);
    }

    return res;

out_destroy:
    fpgaDestroyProperties(&props);
    return res;
}
