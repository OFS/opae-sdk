#ifndef __HSSI_MBOX_H__
#define __HSSI_MBOX_H__
#include <stdint.h>
#include <time.h>

#define TRAFFIC_CTRL_CMD 0x0030
#define READ_CMD  0x00000001ULL
#define WRITE_CMD 0x00000002ULL
#define ACK_TRANS 0x00000004ULL
#define AFU_CMD_SHIFT 32
#define AFU_CMD_MASK  0x0000ffff00000000ULL
#define ACK_TRANS_CLEAR 0xfffffffffffffffbULL

#define TRAFFIC_CTRL_DATA 0x0038
#define READ_DATA_MASK 0x00000000ffffffffULL
#define WRITE_DATA_SHIFT 32
#define WRITE_DATA_MASK 0xffffffff00000000ULL

#define NO_TIMEOUT 0xffffffffffffffffULL

static inline
uint32_t mbox_get_read_data(uint8_t *mmio_base)
{
    return (uint32_t) ( *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_DATA)) &
                        READ_DATA_MASK );
}

static inline
void mbox_set_write_data(uint8_t *mmio_base, uint32_t data)
{
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_DATA)) =
        ((uint64_t)data) << WRITE_DATA_SHIFT;
}

static inline
int mbox_write(uint8_t *mmio_base, uint16_t csr,
               uint32_t data, uint64_t max_ticks)
{
    uint64_t val;
    struct timespec ts;

    mbox_set_write_data(mmio_base, data);

    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) =
        ((uint64_t)csr << AFU_CMD_SHIFT) | WRITE_CMD;

    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (nanosleep(&ts, NULL) != -1 &&
            max_ticks != NO_TIMEOUT) {
            if (!max_ticks)
                return 1;
            --max_ticks;
        }
    } while(!(val & ACK_TRANS));

    val &= ACK_TRANS_CLEAR;
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = val;

    return 0;
}

static inline
int mbox_read(uint8_t *mmio_base, uint16_t csr,
              uint32_t *data, uint64_t max_ticks)
{
    uint64_t val;
    struct timespec ts;

    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) =
        ((uint64_t)csr << AFU_CMD_SHIFT) | READ_CMD;

    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (nanosleep(&ts, NULL) != -1 &&
            max_ticks != NO_TIMEOUT) {
            if (!max_ticks)
                return 1;
            --max_ticks;
        }
    } while(!(val & ACK_TRANS));

    *data = mbox_get_read_data(mmio_base);

    val &= ACK_TRANS_CLEAR;
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = val;

    return 0;
} 

/*
#define TRAFFIC_CTRL_PORT_SEL 0x0040
#define PORT_SEL_MASK 0x0000000000000007ULL
#define QSFP0_LANE0 0x00000000ULL
#define QSFP0_LANE1 0x00000001ULL
#define QSFP0_LANE2 0x00000002ULL
#define QSFP0_LANE3 0x00000003ULL
#define QSFP1_LANE0 0x00000004ULL
#define QSFP1_LANE1 0x00000005ULL
#define QSFP1_LANE2 0x00000006ULL
#define QSFP1_LANE3 0x00000007ULL
*/
#endif // __HSSI_MBOX_H__
