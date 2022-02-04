#!/usr/bin/env python3
""" Extrats section for use in fpgaflash that contains:
    0x010000 to 0x01FFFF     : pfl option_bits
    0x020000 to 0x0381FFFF   : S10 factory image page0
    0x03820000 to 0x03ffffff : Max10 device table
    0x04000000 to 0x077FFFFF : S10 user image page1

    output file has above information starting at addr 0x0 """

from __future__ import absolute_import
import argparse
import logging

START_ADDR = 0x010000
# IMAGE_SIZE = 0x071cc14  # actual size of b178 pre-alpha image
IMAGE_SIZE = 0x077f0000

logging.basicConfig(level=0)  # display all logger messages
LOGGER = logging.getLogger(__name__)


def main(input_file, output_file):
    """ Function reads flashh section of input file and
        writes that section to a new file """

    LOGGER.info("Reading input file: %s" % input_file)
    LOGGER.info("Seaking to: 0x%s" % format(START_ADDR, 'x'))
    LOGGER.info("Reading image size: 0x%s" % format(IMAGE_SIZE, 'x'))
    with open(input_file, "rb") as complete_image_file:
        complete_image_file.seek(START_ADDR)
        fpga_image = complete_image_file.read(IMAGE_SIZE)

    LOGGER.info("Writing file: %s" % output_file)
    with open(output_file, "wb") as output_image_file:
        output_image_file.write(fpga_image)
    LOGGER.info("Finished extracting flash section")


if __name__ == '__main__':
    """ Gets args and call the main function """

    LOGGER.info("---Extraction Flash Section---")
    parser = argparse.ArgumentParser()
    parser.add_argument('input_file')
    parser.add_argument('output_file')
    args = parser.parse_args()

    main(format(args.input_file), format(args.output_file))
