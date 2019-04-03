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

#include <uuid/uuid.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "common_sys.h"



fpga_result sysfs_write_u64(const char *path, uint64_t u)
{
    int fd                     = -1;
    int res                    = 0;
    char buf[SYSFS_PATH_MAX]   = {0};
    int b                      = 0;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        FPGA_MSG("open(%s) failed: %s", path, strerror(errno));
        return FPGA_NOT_FOUND;
    }

    if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
        FPGA_MSG("seek: %s", strerror(errno));
        goto out_close;
    }

    snprintf_s_l(buf, sizeof(buf), "0x%lx", u);

    do {
        res = write(fd, buf + b, sizeof(buf) -b);
        if (res <= 0) {
            FPGA_ERR("Failed to write");
            goto out_close;
        }
        b += res;

        if ((unsigned)b > sizeof(buf) || b <= 0) {
            FPGA_MSG("Unexpected size reading from %s", path);
            goto out_close;
        }

    } while (buf[b - 1] != '\n' && buf[b - 1] != '\0' && (unsigned)b < sizeof(buf));

    close(fd);
    return FPGA_OK;

out_close:
    close(fd);
    return FPGA_NOT_FOUND;
}


// Get port syfs path
fpga_result get_port_sysfs(fpga_handle handle,
                char *sysfs_port)
{

    struct _fpga_token  *_token;
    struct _fpga_handle *_handle  = (struct _fpga_handle *)handle;
    char *p                       = 0;
    int device_instance           = 0;

    if (sysfs_port == NULL) {
        FPGA_ERR("Invalid output pointer");
        return FPGA_INVALID_PARAM;
    }

    if (_handle == NULL) {
        FPGA_ERR("Invalid handle");
        return FPGA_INVALID_PARAM;
    }

    _token = (struct _fpga_token *)_handle->token;
    if (_token == NULL) {
        FPGA_ERR("Token not found");
        return FPGA_INVALID_PARAM;
    }

    p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
    if (NULL == p) {
        FPGA_ERR("Invalid sysfspath in token");
        return FPGA_INVALID_PARAM;
    }
    p = strrchr(_token->sysfspath, '.');
    if (NULL == p) {
        FPGA_ERR("Invalid sysfspath in token");
        return FPGA_INVALID_PARAM;
    }

    device_instance = atoi(p + 1);

    snprintf_s_ii(sysfs_port, SYSFS_PATH_MAX,
        SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT,
        device_instance, device_instance);

    return FPGA_OK;
}

fpga_result sysfs_read_u64(const char *path, uint64_t *u)
{
    int fd                     = -1;
    int res                    = 0;
    char buf[SYSFS_PATH_MAX]   = {0};
    int b                      = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        FPGA_MSG("open(%s) failed", path);
        return FPGA_NOT_FOUND;
    }

    if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
        FPGA_MSG("seek failed");
        goto out_close;
    }

    do {
        res = read(fd, buf+b, sizeof(buf)-b);
        if (res <= 0) {
            FPGA_MSG("Read from %s failed", path);
            goto out_close;
        }
        b += res;
        if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
            FPGA_MSG("Unexpected size reading from %s", path);
            goto out_close;
        }
    } while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

    // erase \n
    buf[b-1] = 0;

    *u = strtoull(buf, NULL, 0);

    close(fd);
    return FPGA_OK;

out_close:
    close(fd);
    return FPGA_NOT_FOUND;
}
// wait caldone
// Wait for calibration to be done
int fi_WaitCalDone(void)
{
    // fi_WaitCalDone
    // Wait for calibration to be done
    uint64_t u64i_PrtData                = 0;
    uint64_t u64i_I                      = 0;
    long int li_sleep_nanoseconds        = 0;
    int      res                         = 0;
    char sysfs_usrpath[SYSFS_PATH_MAX]    = {0};

    // Waiting for fcr PLL calibration not to be busy
    for (u64i_I = 0; u64i_I<1000; u64i_I++)
    { // Poll with 1000 ms timeout

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);
        sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);

        if ((u64i_PrtData & QUCPU_UI64_STS_0_BSY_b61) == 0) break;

        // Sleep 1 ms
        li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
        fv_SleepShort(li_sleep_nanoseconds);
    } // Poll with 1000 ms timeout


    if ((u64i_PrtData & QUCPU_UI64_STS_0_BSY_b61) != 0)
    { // ERROR: calibration busy too long
        res = QUCPU_INT_UCLOCK_WAITCALDONE_ERR_BSY_TO;
    } // ERROR: calibration busy too long

    return(res);
} // fi_WaitCalDone


//fi_AvmmRead
int fi_AvmmRead(uint64_t u64i_AvmmAdr, uint64_t *pu64i_ReadData)
{
    // fi_AvmmRead
    int         i_CmdWrite    = 0;
    uint64_t u64i_WriteData   = 0;
    int         res           = 0;

    // Perform read with common code
    i_CmdWrite = 0;
    u64i_WriteData = 0; // Not used for read
    res = fi_AvmmRWcom(i_CmdWrite, u64i_AvmmAdr, u64i_WriteData, pu64i_ReadData);

    // Return error status
    return(res);
} // fi_AvmmRead

