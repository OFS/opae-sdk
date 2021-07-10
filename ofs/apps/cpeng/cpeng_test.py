import argparse
import os
import time

# allocate buffer

CSR_DATA_SRC = 0x0110
CSR_DATA_DST = 0x0118
CSR_DATA_SIZE = 0x0120
CSR_MRD_START = 0x0128
CSR_HPS2HOST_RSP_SHDW = 0x0140


def wait_for_hps_ready():
    while read_csr64(CSR_HPS2HOST_RSP_SHDW) != 0xF:
        time.sleep(0.1)


def read_from_file(fp):
    size = os.path.getsize(fp.name)
    padded_size = (size + 64) & ~63
    print(f'size: {size}, padded: {padded_size}')
    shared_buffer = allocate_buffer(padded_size)
    shared_buffer.read_file(fp)

    return shared_buffer


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--hps-image', type=argparse.FileType('rb'))

    args = parser.parse_args()
    if args.hps_image:
        wait_for_hps_ready()
        io_buffer = read_from_file(args.hps_image)
        # print(f'iova: 0x{io_buffer.io_address:08x}')
        # print(io_buffer)

        # write_csr64(CSR_DATA_SRC, io_buffer.io_address())
        # write_csr64(CSR_DATA_DST, 0)
        # write_csr64(CSR_DATA_SIZE, io_buffer.size())


if __name__ == '__main__':
    main()
