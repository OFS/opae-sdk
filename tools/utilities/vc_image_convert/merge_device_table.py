#!/usr/bin/env python3
""" splice in max10 device table info at 0x3820000

    0x03820000 to 0x03ffffff : Max10 device table range

    can take Max10 device table under above range """

from __future__ import absolute_import
import argparse
import logging
import os

MAX10_TABLE_START = 0x03820000
MAX10_TABLE_SIZE = 0x07e0000
MAX10_TABLE_FILENAME = "max10_device_table.bin"

logging.basicConfig(level=0)  # display all logger messages
LOGGER = logging.getLogger(__name__)


def main(input_file, dtb_file):
    """ function reads and checks max10_device_table.bin to ensure
        the file is under the alloted size
        the section is written into the table section of the input file """

    LOGGER.info("Reading: %s" % dtb_file)
    with open(dtb_file, "rb") as max_table_file:
        LOGGER.info("max10_device_table.bin size: %x" %
                    os.path.getsize(max_table_file.name))
        LOGGER.info("Max max10 table size: %x" % MAX10_TABLE_SIZE)
        if (os.path.getsize(max_table_file.name) > MAX10_TABLE_SIZE):
            raise Exception(LOGGER.error("max10_device_table.bin is too big"))
        max10_table = max_table_file.read()

    LOGGER.info("Writing file: %s" % input_file)
    with open(input_file, "rb+") as rpd_file:
        rpd_file.seek(MAX10_TABLE_START)
        rpd_file.write(bytearray(max10_table))
    LOGGER.info("Done merging Max10 device table")


if __name__ == '__main__':

    LOGGER.info("--- Merging Max10 table ---")
    parser = argparse.ArgumentParser()
    parser.add_argument('input_file')
    parser.add_argument('-d', '--dtb', default=MAX10_TABLE_FILENAME)
    args = parser.parse_args()

    main(format(args.input_file), format(args.dtb))
