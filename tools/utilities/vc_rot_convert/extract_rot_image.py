#!/usr/bin/env python

import argparse
import struct
import array
import os
import logging


logging.basicConfig(level=0)  # display all logger messages
LOGGER = logging.getLogger(__name__)
BUF_LEN = 128


def main(input_file, output_file):
    """ Function reads flashh section of input file and
        writes that section to a new file """

    LOGGER.info("Reading input file: %s" % input_file)
    with open(input_file, "rb") as complete_image_file:
        complete_image_file.seek(0, os.SEEK_END)
        file_size = complete_image_file.tell()
        LOGGER.info("Input file size: {}".format(file_size))

        # search payload start address in POF
        offset = 0
        length = BUF_LEN
        pos = 0
        found = False
        repeat_count = 0
        while offset < file_size:
            buf = array.array('B')
            remain = file_size - offset
            length = BUF_LEN if remain > BUF_LEN else remain
            complete_image_file.seek(offset, os.SEEK_SET)
            buf.fromfile(complete_image_file, length)
            unpacked_data = struct.unpack_from('B' * length, buf)
            for i, d in enumerate(unpacked_data):
                if d == 0xff:
                    if repeat_count == 0:
                        pos = offset + i
                    repeat_count += 1
                    if repeat_count > 16:
                        found = True
                        break
                else:
                    repeat_count = 0
                    pos = None
            if found:
                break
            offset += length

        if not found:
            LOGGER.info("Start address search failed")
            return

        LOGGER.info('Start address {:#x}'.format(pos))

        # check option bits for data offset and length
        complete_image_file.seek(pos+0x10000, os.SEEK_SET)
        buf = array.array('B')
        buf.fromfile(complete_image_file, 4)
        unpacked_data = struct.unpack_from('HH', buf)
        offset = pos + (unpacked_data[0] << 12)
        length = unpacked_data[1] << 12
        LOGGER.info('Data offset {:#x} with length {:#x}'.format(offset,
                                                                 length))

        # extract data
        complete_image_file.seek(offset, os.SEEK_SET)
        fpga_image = complete_image_file.read(length)

    LOGGER.info("Writing file: %s" % output_file)
    with open(output_file, "wb") as output_image_file:
        output_image_file.write(fpga_image)
    LOGGER.info("Finished extracting data section")


if __name__ == '__main__':
    """ Gets args and call the main function """

    LOGGER.info("---Extraction Flash Section---")
    parser = argparse.ArgumentParser()
    parser.add_argument('input_file')
    parser.add_argument('output_file')
    args = parser.parse_args()

    main(format(args.input_file), format(args.output_file))
