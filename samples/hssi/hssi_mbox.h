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
#ifndef __HSSI_MBOX_H__
#define __HSSI_MBOX_H__
#ifdef __cplusplus
#include <iostream>
#include <string>
#include <sstream>
#endif // __cplusplus
#include <stdint.h>
#include <time.h>

#if 0
#define TRAFFIC_CTRL_CMD  0x0030
#define READ_CMD          0x00000001ULL
#define WRITE_CMD         0x00000002ULL
#define ACK_TRANS         0x00000004ULL
#define AFU_CMD_SHIFT     32
#define AFU_CMD_MASK      0x0000ffff00000000ULL
#define ACK_TRANS_CLEAR   0xffff0000fffffff8ULL

#define TRAFFIC_CTRL_DATA 0x0038
#define READ_DATA_MASK    0x00000000ffffffffULL
#define WRITE_DATA_SHIFT  32
#define WRITE_DATA_MASK   0xffffffff00000000ULL
#else
#define TRAFFIC_CTRL_CMD  0x0030
#define READ_CMD          0x00000001
#define WRITE_CMD         0x00000002
#define ACK_TRANS         0x00000004

#define TRAFFIC_CTRL_DATA 0x0038
#endif

#define NO_TIMEOUT        0xffffffffffffffffULL

#ifdef __cplusplus
template <typename X>
std::string int_to_hex(X x)
{
  std::stringstream ss;
  ss << "0x" <<
    std::setfill('0') <<
    std::setw(2 * sizeof(X)) <<
    std::hex << x;
  return ss.str();
}
#endif // __cplusplus

#if 0
static inline
int mbox_write(uint8_t *mmio_base, uint16_t csr,
               uint32_t data, uint64_t max_ticks)
{
    uint64_t val;
    struct timespec ts;
    uint64_t ticks;

    val = (((uint64_t)data) << WRITE_DATA_SHIFT);
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_DATA)) = val;
    std::cout << "write a " << int_to_hex(val) << std::endl;

    val = (((uint64_t)csr) << AFU_CMD_SHIFT) | WRITE_CMD;
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = val;
    std::cout << "write b " << int_to_hex(val) << std::endl;

    ticks = max_ticks;
    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (val & ACK_TRANS)
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (!(val & ACK_TRANS));

    ticks = max_ticks;
    do
    {
        *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = ACK_TRANS;
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (!(val & ACK_TRANS))
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (val & ACK_TRANS);

    return 0;
}

static inline
int mbox_read(uint8_t *mmio_base, uint16_t csr,
              uint32_t *data, uint64_t max_ticks)
{
    uint64_t val;
    struct timespec ts;
    uint64_t ticks;

    val = (((uint64_t)csr) << AFU_CMD_SHIFT) | READ_CMD;
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = val;

    ticks = max_ticks;
    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (val & ACK_TRANS)
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (!(val & ACK_TRANS));

    val = *(volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_DATA);
    *data = (uint32_t)val;
 
    ticks = max_ticks;
    do
    {
        *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = ACK_TRANS;
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (!(val & ACK_TRANS))
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (val & ACK_TRANS);

    return 0;
} 

#else

static inline
int mbox_write(uint8_t *mmio_base, uint16_t csr,
               uint32_t data, uint64_t max_ticks)
{
    uint32_t val;
    struct timespec ts;
    uint64_t ticks;

    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_DATA + 4)) = data;
    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD + 4)) = (uint32_t)csr;
    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = WRITE_CMD;

    ticks = max_ticks;
    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (val & ACK_TRANS)
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (!(val & ACK_TRANS));

    ticks = max_ticks;
    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = 0;
    do
    {
        val = *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (!(val & ACK_TRANS))
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (val & ACK_TRANS);

    return 0;
}

static inline
int mbox_read(uint8_t *mmio_base, uint16_t csr,
              uint32_t *data, uint64_t max_ticks)
{
    uint32_t val;
    struct timespec ts;
    uint64_t ticks;

    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD + 4)) = (uint32_t)csr;
    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = READ_CMD;

    ticks = max_ticks;
    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (val & ACK_TRANS)
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (!(val & ACK_TRANS));

    *data = *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_DATA));

    ticks = max_ticks;
    *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = 0;
    do
    {
        val = *((volatile uint32_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (!(val & ACK_TRANS))
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                return 1;
            --ticks;
        }
    } while (val & ACK_TRANS);

    return 0;
}

#endif

#endif // __HSSI_MBOX_H__
