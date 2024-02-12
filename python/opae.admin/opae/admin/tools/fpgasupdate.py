#! /usr/bin/env python3
# Copyright(c) 2019-2023, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Program PAC firmware"""

from __future__ import absolute_import
import argparse
import os
import sys
import struct
import re
import fcntl
import array
import binascii
import string
import json
import uuid
import subprocess
import time
from datetime import datetime
import signal
import errno
import logging
from ctypes import cdll
from ctypes.util import find_library
import tempfile
import shutil
from opae.admin.fpga import fpga
from opae.admin.utils.progress import progress
from opae.admin.version import pretty_version
from opae.admin.sysfs import sysfs_device, sysfs_node

if sys.version_info[0] == 2:
    input = raw_input  # noqa pylint: disable=E0602

    def uuid_from_bytes(blob):
        return uuid.UUID(bytes=list(reversed(blob)))
else:
    def uuid_from_bytes(blob):
        return uuid.UUID(bytes=blob)

DEFAULT_BDF = 'ssss:bb:dd.f'

VALID_GBS_GUID = uuid.UUID('58656f6e-4650-4741-b747-425376303031')

BLOCK0_TYPE_STATIC_REGION = 0
BLOCK0_TYPE_BMC = 1
BLOCK0_TYPE_GBS = 2

BLOCK0_SUBTYPE_UPDATE = 0x0000
BLOCK0_SUBTYPE_CANCELLATION = 0x0100
BLOCK0_SUBTYPE_ROOT_KEY_HASH_256 = 0x0200
BLOCK0_SUBTYPE_ROOT_KEY_HASH_384 = 0x0300

BLOCK0_CONTYPE_MASK = 0x00ff
BLOCK0_CONSUBTYPE_MASK = 0xff00

IOCTL_FPGA_LOAD_WRITE = 0x4018b900
IOCTL_FPGA_LOAD_STATUS = 0x8010b901
IOCTL_FPGA_LOAD_CANCEL = 0xb902

FPGA_PROG_IDLE = 0
FPGA_PROG_STARTING = 1
FPGA_PROG_PREPARING = 2
FPGA_PROG_WRITING = 3
FPGA_PROG_PROGRAMMING = 4
FPGA_PROG_MAX = 5

FPGA_ERR_NONE = 0
FPGA_ERR_HW_ERROR = 1
FPGA_ERR_TIMEOUT = 2
FPGA_ERR_CANCELED = 3
FPGA_ERR_BUSY = 4
FPGA_ERR_INVALID_SIZE = 5
FPGA_ERR_RW_ERROR = 6
FPGA_ERR_WEAROUT = 7
FPGA_ERR_MAX = 8

# Values for ConType field in Block0 of output binary from PACSign utility.
CONTENT_FACTORY = 3

# bytes/sec when staging is flash
FLASH_COPY_BPS = 43000.0
# bytes/sec when staging area is dram
DRAM_COPY_BPS = 92000.0
DRAM_COPY_OFFSET = 42.0

LOG = logging.getLogger()

LOG_NAMES_TO_LEVELS = {
    'debug': logging.DEBUG,
    'info': logging.INFO,
    'warning': logging.WARNING,
    'error': logging.ERROR,
    'critical': logging.CRITICAL
}

TMPFILE = ''


def parse_args():
    """Parses command line arguments
    """
    parser = argparse.ArgumentParser()

    file_help = 'self-describing, signed file to update PAC card.'
    parser.add_argument('file', type=argparse.FileType('rb'), nargs='?',
                        help=file_help)

    bdf_help = 'bdf of device to program (e.g. 04:00.0 or 0000:04:00.0).' \
               ' Optional when one device in system.'
    parser.add_argument('bdf', nargs='?', default=DEFAULT_BDF, help=bdf_help)

    log_levels = ['debug', 'info', 'warning', 'error', 'critical']
    parser.add_argument('--log-level', choices=log_levels,
                        default='info', help='log level to use')

    parser.add_argument('-y', '--yes', default=False, action='store_true',
                        help='answer Yes to all confirmation prompts')

    parser.add_argument('-v', '--version', action='version',
                        version=f"%(prog)s {pretty_version()}",
                        help='display version information and exit')

    return parser, parser.parse_args()


def decode_gbs_header(infile):
    """Reads and decodes a Green BitStream header.

    infile - a valid, open file object specified on the command line.

    Returns a dictionary of the broken out fields for the header.
    """
    orig_pos = infile.tell()

    infile.seek(0, os.SEEK_END)
    file_size = infile.tell()

    infile.seek(0, os.SEEK_SET)

    hdr_size = 20

    if file_size < hdr_size:
        LOG.warning('%s does not meet the minimum '
                    'file size requirement of %d',
                    infile.name, hdr_size)
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    hdr = infile.read(hdr_size)

    # offset  size  name
    # 0x000     16  valid_gbs_guid
    # 0x010      4  metadata_length

    valid_gbs_guid = uuid_from_bytes(hdr[:16])

    if valid_gbs_guid != VALID_GBS_GUID:
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    unpacked_hdr = struct.unpack_from('I'*4 + 'I', hdr)

    valid_gbs_guid_str = str(valid_gbs_guid).replace('-', '')
    valid_gbs_bytes = [valid_gbs_guid_str[i:i+2] for i in range(0, 32, 2)]
    pretty_valid_gbs = ''
    for a_byte in reversed(valid_gbs_bytes):
        a_char = chr(int(a_byte, 16))
        if a_char in string.printable:
            pretty_valid_gbs = pretty_valid_gbs + a_char
        else:
            pretty_valid_gbs = pretty_valid_gbs + '?'

    LOG.debug('GBS guid: %s', valid_gbs_guid)
    LOG.debug('pretty GBS guid: %s', pretty_valid_gbs)

    metadata_length = unpacked_hdr[4]
    hdr_size += metadata_length

    if file_size < hdr_size:
        LOG.warning('%s does not meet the minimum '
                    'file size requirement of %d',
                    infile.name, hdr_size)
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    metadata = infile.read(metadata_length).decode()
    LOG.debug('metadata: %s', metadata)

    try:
        json_obj = json.loads(metadata)
    except ValueError as exc:
        LOG.info(exc)
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    if 'afu-image' not in json_obj:
        LOG.error('No "afu-image" key in JSON')
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    afu_image = json_obj['afu-image']

    if 'interface-uuid' not in afu_image:
        LOG.error('No "interface-uuid" key in JSON')
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    interface_uuid = afu_image['interface-uuid']
    LOG.debug('interface-uuid: %s', interface_uuid)

    return {'valid_gbs_guid': valid_gbs_guid,
            'pretty_gbs_guid': pretty_valid_gbs,
            'metadata': metadata,
            'interface-uuid': interface_uuid}


def do_partial_reconf(addr, filename):
    """Call out to fpgaconf for Partial Reconfiguration.

    addr - the canonical ssss:bb:dd.f of the device to PR.
    filename - the GBS file path.

    returns a 2-tuple of the process exit status and a message.
    """
    conf_args = ['fpgaconf', '--segment', '0x' + addr[:4],
                 '--bus', '0x' + addr[5:7],
                 '--device', '0x' + addr[8:10],
                 '--function', '0x' + addr[11:], filename]

    LOG.debug('command: %s', ' '.join(conf_args))

    try:
        output = subprocess.check_output(conf_args, stderr=subprocess.STDOUT)
        output = output.decode(sys.getdefaultencoding())
    except subprocess.CalledProcessError as exc:
        return (exc.returncode,
                exc.output.decode(sys.getdefaultencoding()) +
                '\nPartial Reconfiguration failed')

    return (0, output + '\nPartial Reconfiguration OK')


def decode_auth_block0(infile, prompt):
    """Reads and decodes an authentication block 0.

    infile - a valid, open file object specified on the command line.
    prompt - a boolean; set to true to display prompt

    Returns a dictionary of the broken out fields for block0.
    """
    orig_pos = infile.tell()

    infile.seek(0, os.SEEK_END)
    file_size = infile.tell()

    block0_size = 128
    block0_magic = 0xb6eafd19

    #                          block1 payload
    min_file_size = block0_size + 896 + 128

    if file_size < min_file_size:
        LOG.warning('%s does not meet the minimum file size '
                    'requirement of %d',
                    infile.name, min_file_size)
        infile.seek(orig_pos, os.SEEK_SET)
        return None

    infile.seek(0, os.SEEK_SET)

    block0 = infile.read(block0_size)
    infile.seek(orig_pos, os.SEEK_SET)

    # offset  size  name
    # 0x000      4  Magic
    # 0x004      4  ConLen
    # 0x008      4  ConType
    # 0x00c      4  Reserved
    # 0x010     32  Hash256
    # 0x030     48  Hash384
    # 0x060     32  Reserved

    unpacked_block0 = struct.unpack_from('I'*4 + '32s' + '48s', block0)

    if unpacked_block0[0] != block0_magic:
        return None

    h256 = binascii.hexlify(unpacked_block0[4])
    h384 = binascii.hexlify(unpacked_block0[5])

    infile.seek(0, os.SEEK_SET)

    res = {'Magic': unpacked_block0[0],
           'ConLen': unpacked_block0[1],
           'ConType': unpacked_block0[2],
           'Hash256': h256,
           'Hash384': h384}

    LOG.debug('hash256: %s', res['Hash256'])
    LOG.debug('hash384: %s', res['Hash384'])

    contype = res['ConType'] & BLOCK0_CONTYPE_MASK
    contypes = {BLOCK0_TYPE_STATIC_REGION: 'Static Region',
                BLOCK0_TYPE_BMC: 'BMC Image',
                BLOCK0_TYPE_GBS: 'Green BitStream'}

    consubtype = res['ConType'] & BLOCK0_CONSUBTYPE_MASK
    consubtypes = {BLOCK0_SUBTYPE_UPDATE: 'Update',
                   BLOCK0_SUBTYPE_CANCELLATION: 'Key Cancellation',
                   BLOCK0_SUBTYPE_ROOT_KEY_HASH_256: 'Root Entry Hash 256',
                   BLOCK0_SUBTYPE_ROOT_KEY_HASH_384: 'Root Entry Hash 384'}

    LOG.debug('file type: %s (%s)',
              contypes.get(contype, '<unknown>'),
              consubtypes.get(consubtype, '<unknown>'))

    warn_on = [BLOCK0_SUBTYPE_CANCELLATION,
               BLOCK0_SUBTYPE_ROOT_KEY_HASH_256,
               BLOCK0_SUBTYPE_ROOT_KEY_HASH_384]

    while consubtype in warn_on and prompt:
        msg = '*** Programming a Root Entry Hash or Key Cancellation ' \
              'cannot be undone! Continue? [yes/No]> '
        ans = input(msg)
        if ans.lower().startswith('yes'):
            break
        if len(ans) == 0 or ans.lower().startswith('n'):
            LOG.info('Operation canceled by user.')
            sys.exit(1)

    return res


def canonicalize_bdf(bdf):
    """Verifies the given PCIe address.

    bdf - a string representing the PCIe address. It must be of
          the form bb:dd.f or ssss:bb:dd.f.

    returns None if bdf does not have the proper form. Otherwise
    returns the canonical form as a string.
    """
    abbrev_pcie_addr_pattern = r'(?P<bus>[\da-f]{2}):' \
                               r'(?P<device>[\da-f]{2})\.' \
                               r'(?P<function>\d)'
    pcie_addr_pattern = r'(?P<segment>[\da-f]{4}):' + abbrev_pcie_addr_pattern

    abbrev_regex = re.compile(abbrev_pcie_addr_pattern, re.IGNORECASE)

    match = abbrev_regex.match(bdf)
    if match:
        return '0000:' + bdf

    regex = re.compile(pcie_addr_pattern, re.IGNORECASE)
    match = regex.match(bdf)
    if match:
        return bdf

    return None


def ioctl_status(sec_dev):
    """Call the status ioctl

    sec_dev - an integer file descriptor to the os.open()'ed secure device file

    returns remaining size, progress code, progress code at error, error code
    """
    # struct fpga_image_status {
    # /* Output */
    # __u32 remaining_size;			/* size remaining to transfer */
    # enum fpga_image_prog progress;		/* current phase of image load */
    # enum fpga_image_prog err_progress;	/* progress at time of error */
    # enum fpga_image_err err_code;		/* error code */
    # };
    status_header = array.array('B', [0] * 32)
    fcntl.ioctl(sec_dev, IOCTL_FPGA_LOAD_STATUS, status_header, True)
    remaining_size, prog, err_prog, err_code = \
        struct.unpack_from('IIII', status_header)
    return remaining_size, prog, err_prog, err_code


def fpga_err_to_string(err_code):
    """Returns a string representation of the FPGA error code
    """
    if err_code == FPGA_ERR_HW_ERROR:
        return "hw_error"
    if err_code == FPGA_ERR_TIMEOUT:
        return "timeout"
    if err_code == FPGA_ERR_CANCELED:
        return "canceled"
    if err_code == FPGA_ERR_BUSY:
        return "busy"
    if err_code == FPGA_ERR_INVALID_SIZE:
        return "invalid_size"
    if err_code == FPGA_ERR_RW_ERROR:
        return "rw_error"
    if err_code == FPGA_ERR_WEAROUT:
        return "flash_wearout"
    return ""


class SecureUpdateError(Exception):
    """Secure update exception"""
    def __init__(self, arg):
        super().__init__(self)
        self.errno, self.strerror = arg


def update_fw_ioctl(sec_dev, infile, pac):
    """Writes firmware to secure device using ioctl calls.

    sec_dev - an integer file descriptor to the os.open()'ed secure device file
    infile - the image to write to the FPGA
    pac - opae fpga module

    returns a 2-tuple of the process exit status and a message.
    """

    orig_pos = infile.tell()
    infile.seek(0, os.SEEK_END)
    payload_size = infile.tell() - orig_pos

    LOG.info('updating from file %s with size %d',
             infile.name, payload_size)

    file_buf = array.array('B')
    infile.seek(orig_pos, os.SEEK_SET)
    file_buf.fromfile(infile, payload_size)
    infile.close()
    addr, _ = file_buf.buffer_info()

    # Create the eventfd file descriptor
    clib = find_library("c")
    if clib is None:
        return errno.EINVAL, 'C library not found'
    libc = cdll.LoadLibrary(clib)
    evtfd = libc.eventfd(0, 0)

    write_header = array.array('B', [0] * 32)
    #  struct fpga_image_write {
    # 	/* Input */
    # 	__u32 flags;		/* Zero for now */
    # 	__u32 size;		/* Data size (in bytes) to be written */
    # 	__s32 evtfd;		/* File descriptor for completion signal */
    # 	__u64 buf;		/* User space address of source data */
    # };
    struct.pack_into('IIIQ', write_header, 0, 0, payload_size, evtfd, addr)

    LOG.info('waiting for idle')
    retries = 0
    timeout = 1.0
    max_retries = 60 * 5
    _, prog_code, _, _ = ioctl_status(sec_dev)
    while prog_code != FPGA_PROG_IDLE:
        time.sleep(timeout)
        retries += 1
        if retries > max_retries:
            return errno.ETIMEDOUT, 'Secure update timed out'
        _, prog_code, _, _ = ioctl_status(sec_dev)

    # IOCTL Call to kick off the write
    fcntl.ioctl(sec_dev, IOCTL_FPGA_LOAD_WRITE, write_header, True)

    LOG.info('preparing image file')
    retries = 0
    max_retries = 60 * 5
    _, prog_code, _, _ = ioctl_status(sec_dev)
    while prog_code in [FPGA_PROG_STARTING, FPGA_PROG_PREPARING]:
        time.sleep(timeout)
        retries += 1
        if retries > max_retries:
            return errno.ETIMEDOUT, 'Secure update timed out'
        _, prog_code, _, _ = ioctl_status(sec_dev)

    _, prog_code, _, err_code = ioctl_status(sec_dev)
    if prog_code == FPGA_PROG_IDLE and err_code != FPGA_ERR_NONE:
        return 1, fpga_err_to_string(err_code)

    progress_cfg = {}
    level = min([handler.level for handler in LOG.handlers])
    if level < logging.INFO:
        progress_cfg['log'] = LOG.debug
    else:
        progress_cfg['stream'] = sys.stdout

    LOG.info('writing image file')
    retries = 0
    max_retries = 60 * 60 * 2
    with progress(bytes=payload_size, **progress_cfg) as prg:
        remaining_size, _, _, _ = ioctl_status(sec_dev)
        while int(remaining_size) > 0:
            time.sleep(timeout)
            retries += 1
            if retries > max_retries:
                return errno.ETIMEDOUT, 'Secure update timed out'
            prg.update(payload_size - int(remaining_size))
            remaining_size, _, _, _ = ioctl_status(sec_dev)

    _, prog_code, _, err_code = ioctl_status(sec_dev)
    if prog_code == FPGA_PROG_IDLE and err_code != FPGA_ERR_NONE:
        return 1, fpga_err_to_string(err_code)

    if pac.fme.have_node('tcm'):
        estimated_time = payload_size / DRAM_COPY_BPS + DRAM_COPY_OFFSET
    else:
        estimated_time = payload_size / FLASH_COPY_BPS
    # over-estimate by 1.5 to account for flash performance degradation
    estimated_time *= 1.5

    LOG.info('programming image file')
    interrupt_flag = False
    retries = 0
    max_retries = 60 * 60 * 3
    with progress(time=estimated_time, **progress_cfg) as prg:
        while prog_code in [FPGA_PROG_WRITING, FPGA_PROG_PROGRAMMING]:
            try:
                time.sleep(timeout)
                retries += 1
                if retries > max_retries:
                    return errno.ETIMEDOUT, 'Secure update timed out'
                prg.tick()
                _, prog_code, _, _ = ioctl_status(sec_dev)
                if prog_code is FPGA_PROG_PROGRAMMING and interrupt_flag:
                    LOG.warning('Ignoring Ctrl+C: programming phase is not interruptable')
                    interrupt_flag = False
            except KeyboardInterrupt:
                fcntl.ioctl(sec_dev, IOCTL_FPGA_LOAD_CANCEL)
                interrupt_flag = True

    _, prog_code, _, err_code = ioctl_status(sec_dev)
    if prog_code == FPGA_PROG_IDLE and err_code != FPGA_ERR_NONE:
        return 1, fpga_err_to_string(err_code)

    LOG.info('update of %s complete', pac.pci_node.pci_address)

    return 0, 'Secure update OK'


def update_fw_sysfs(infile, pac):
    """Writes firmware to secure device.

    infile - the image to write to the FPGA
    pac - opae fpga module

    returns a 2-tuple of the process exit status and a message.
    """

    orig_pos = infile.tell()
    infile.seek(0, os.SEEK_END)
    payload_size = infile.tell() - orig_pos

    sec_dev = pac.upload_dev

    # This function supports two variations of the sysfs-based loading mechanism.
    # The first is identified by the presence of "update/filename" in the path.

    if sec_dev.find_one(os.path.join('update', 'filename')):
        legacy = True
        infile.close()
        error = sec_dev.find_one(os.path.join('update', 'error'))
        filename = sec_dev.find_one(os.path.join('update', 'filename'))
        size = sec_dev.find_one(os.path.join('update', 'remaining_size'))
        status = sec_dev.find_one(os.path.join('update', 'status'))
    else:
        legacy = False
        infile.seek(0, os.SEEK_SET)
        error = sec_dev.find_one('error')
        size = sec_dev.find_one('remaining_size')
        status = sec_dev.find_one('status')

        data = sec_dev.find_one('data')
        loading = sec_dev.find_one('loading')

    if legacy:
        fw_path = os.path.join(os.sep, 'usr', 'lib', 'firmware')
        if not os.path.isdir(fw_path):
            LOG.error("Can't find %s", fw_path)
            return 1, 'Secure update failed'

        intel_fw_path = os.path.join(fw_path, 'intel')
        if not os.path.isdir(intel_fw_path):
            try:
                os.mkdir(intel_fw_path, 0o755)
            except OSError:
                LOG.error("Can't create %s", intel_fw_path)
                return 1, 'Secure update failed'

    LOG.info('updating from file %s with size %d',
             infile.name, payload_size)

    if legacy:
        tfile = tempfile.NamedTemporaryFile(dir=intel_fw_path, delete=False)
        tfile.close()

        shutil.copy(infile.name, tfile.name)
        global TMPFILE
        TMPFILE = tfile.name

    LOG.info('waiting for idle')
    retries = 0
    timeout = 1.0
    max_retries = 60 * 5
    while status.value != 'idle':
        time.sleep(timeout)
        retries += 1
        if retries >= max_retries:
            if legacy:
                os.remove(tfile.name)
            return errno.ETIMEDOUT, 'Secure update timed out'

    if legacy:
        filename.value = os.path.join('intel', os.path.basename(tfile.name))
    else:
        loading.value = '1'
        data.bin_value = infile.read()
        loading.value = '0'

    LOG.info('preparing image file')

    retries = 0
    max_retries = 60 * 5
    # read_file is now deprecated. Leaving it in for backwards compat.
    while status.value in ['read_file', 'receiving', 'reading', 'preparing']:
        time.sleep(timeout)
        retries += 1
        if retries >= max_retries:
            if legacy:
                os.remove(tfile.name)
            return errno.ETIMEDOUT, 'Secure update timed out'

    if legacy:
        os.remove(tfile.name)
        TMPFILE = ''

    if status.value == 'idle':
        e = error.value
        if e:
            return 1, e

    progress_cfg = {}
    level = min([handler.level for handler in LOG.handlers])
    if level < logging.INFO:
        progress_cfg['log'] = LOG.debug
    else:
        progress_cfg['stream'] = sys.stdout

    LOG.info('writing image file')

    retries = 0
    max_retries = 60 * 60 * 2
    with progress(bytes=payload_size, **progress_cfg) as prg:
        while int(size.value) > 0 and status.value != 'idle':
            time.sleep(timeout)
            retries += 1
            if retries >= max_retries:
                return errno.ETIMEDOUT, 'Secure update timed out'
            prg.update(payload_size - int(size.value))

    if status.value == 'idle':
        e = error.value
        if e:
            return 1, e

    if pac.fme.have_node('tcm'):
        estimated_time = payload_size / DRAM_COPY_BPS + DRAM_COPY_OFFSET
    else:
        estimated_time = payload_size / FLASH_COPY_BPS
    # over-estimate by 1.5 to account for flash performance degradation
    estimated_time *= 1.5

    LOG.info('programming image file')

    retries = 0
    max_retries = 60 * 60 * 3
    if estimated_time < timeout:
       estimated_time = timeout

    with progress(time=estimated_time, **progress_cfg) as prg:
        while status.value in ('writing', 'programming', 'transferring'):
            time.sleep(timeout)
            retries += 1
            if retries >= max_retries:
                return errno.ETIMEDOUT, 'Secure update timed out'
            prg.tick()

    if status.value == 'idle':
        e = error.value
        if e:
            return 1, e

    LOG.info('update of %s complete', pac.pci_node.pci_address)

    return 0, 'Secure update OK'


def sig_handler(signum, frame):
    """raise exception for SIGTERM
    """
    raise SecureUpdateError((1, 'Caught SIGTERM'))


class MyLogFormatter(logging.Formatter):
    '''Custom logging.Formatter object.
       Overrides formatTime() to limit the fractional seconds
       field to hundredths.'''
    def formatTime(self, record, datefmt=None):
        dt = super().formatTime(record, datefmt)
        front = dt[:-4]
        end = dt[-4:]
        return front + '.' + end[1:3]


def main():
    """The main entry point."""
    parser, args = parse_args()

    if args.file is None:
        print('Error: file is a required argument\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    LOG.setLevel(logging.NOTSET)

    log_fmt = ('[%(asctime)-15s] [%(levelname)-8s] '
               '%(message)s')
    log_hndlr = logging.StreamHandler(sys.stdout)
    log_hndlr.setFormatter(MyLogFormatter(log_fmt))
    log_hndlr.setLevel(LOG_NAMES_TO_LEVELS[args.log_level])
    LOG.addHandler(log_hndlr)

    signal.signal(signal.SIGTERM, sig_handler)

    LOG.debug('fw file: %s', args.file.name)
    LOG.debug('addr: %s', args.bdf)

    stat = 1
    mesg = 'Secure update failed'
    gbs_hdr = None
    use_ioctl = True

    blk0 = decode_auth_block0(args.file, not args.yes)
    if blk0 is None:
        gbs_hdr = decode_gbs_header(args.file)
        if gbs_hdr is None:
            LOG.error('Unknown file format in %s', args.file.name)
            sys.exit(1)

    pac = None
    if args.bdf == DEFAULT_BDF:
        # Address wasn't specified. Enumerate all PAC's.
        if gbs_hdr:
            pr_guid = gbs_hdr['interface-uuid'].replace('-', '')
            pacs = fpga.enum([{'fme.pr_interface_id': pr_guid}])
        else:
            pacs = fpga.enum()

        if not pacs:
            LOG.error('No suitable PAC found.')
            sys.exit(1)
        elif len(pacs) > 1:
            LOG.error('More than one PAC found. '
                      'Please specify an %s', DEFAULT_BDF)
            sys.exit(1)
        pac = pacs[0]
    else:
        # Enumerate for a specific device.
        canon_bdf = canonicalize_bdf(args.bdf)
        if not canon_bdf:
            LOG.error('%s is not a valid PCIe address. '
                      'Use %s', args.bdf, DEFAULT_BDF)
            sys.exit(1)

        pacs = fpga.enum([{'pci_node.pci_address': canon_bdf}])
        if not pacs:
            LOG.error('Failed to find PAC at %s', canon_bdf)
            sys.exit(1)

        pac = pacs[0]

        if gbs_hdr:
            # verify the PR interface uuid
            fme = pac.fme
            if fme:
                pr_guid = gbs_hdr['interface-uuid'].replace('-', '')
                if fme.pr_interface_id.replace('-', '') != pr_guid:
                    LOG.error('PR interface uuid mismatch.')
                    sys.exit(1)

    # The BMC disallows updating the factory image if the current boot-page is also 'factory'.
    # The idea is to always have at least one known-good image in the flash so that
    # you can recover from subsequent bad images.
    # The BMC checks for this condition but does not, at the moment, report any useful error details.
    # We simply get a generic error back and can't report any detail to the user.
    # So we explicitly check for this condition and disallow it here with a descriptive message.

    # The bootpage is read from the fpga_boot_image sysfs entry. The 'fme' object has many sysfs_nodes
    # for various items, including the boot_page, so we use that here. It simply returns a string
    # indicating the boot page: fpga_factory, fpga_user1, or fpga_user2

    # But there are 2 conditions where we may skip this check altogether.
    # 1. If the boot_page entry is not available
    # 2. If blk0 is absent (i.e. the binary is a '.gbs' rather than the output of PACSign)

    boot_page = pac.fme.boot_page

    if (boot_page is None) or (blk0 is None):
        LOG.debug('Attemping to check if boot-page==factory and flash-target==factory...')
        if boot_page is None:
            LOG.debug('But could not find **/fpga_boot_image sysfs entry, which tells us the boot-page. Skipping check.')
        if blk0 is None:
            LOG.debug('But could not find Auth Block0 in the binary, therefore this may be a .gbs binary. Skipping check.')
    else:
        LOG.debug ("Boot page sysfs path: %s\n", boot_page.sysfs_path)
        LOG.debug ("Boot page value: %s\n", boot_page.value)
        LOG.debug ('Block0 ConType: %s\n', blk0['ConType'])

        # The binary is produced by the PACSign utility. 
        # CONTENT_FACTORY is the enum that PACSign inserts into the block0 region of
        # the binary to indicate that the factory image is targeted. ConType refers to 'content type'
        # and indicates if the binary is factoryPR, static region, BMC-related etc.
        if ((boot_page.value == 'fpga_factory') and (blk0['ConType'] == CONTENT_FACTORY)):
            LOG.error('Secure update failed. Cannot update factory image when current boot-page is also factory.')
            sys.exit(1)

    LOG.warning('Update starting. Please do not interrupt.')

    start = datetime.now()

    if gbs_hdr is not None:
        args.file.close()
        stat, mesg = do_partial_reconf(pac.pci_node.pci_address,
                                       args.file.name)
    elif blk0 is not None:
        # Check for 'update/filename' to determine if we use sysfs or ioctl
        if (pac.upload_dev.find_one(os.path.join('update', 'filename')) or
            pac.upload_dev.find_one('loading')):
            use_ioctl = False

        sec_dev = pac.upload_dev
        if not sec_dev:
            LOG.error('Failed to find secure '
                      'device for PAC %s', pac.pci_node.pci_address)
            sys.exit(1)

        LOG.debug('Found secure device for PAC '
                  '%s', pac.pci_node.pci_address)

        with pac.fme:
            if use_ioctl:
                with sec_dev as fd:
                    try:
                        stat, mesg = update_fw_ioctl(fd, args.file, pac)
                    except SecureUpdateError as exc:
                        stat, mesg = exc.errno, exc.strerror
                    except KeyboardInterrupt:
                        fcntl.ioctl(fd, IOCTL_FPGA_LOAD_CANCEL)
                        stat, mesg = 1, 'Interrupted'
                    except OSError:
                        stat, mesg = 1, 'OS Error'
            else:
                try:
                    stat, mesg = update_fw_sysfs(args.file, pac)
                except SecureUpdateError as exc:
                    stat, mesg = exc.errno, exc.strerror
                except KeyboardInterrupt:
                    global TMPFILE
                    if TMPFILE and os.path.isfile(TMPFILE):
                        os.remove(TMPFILE)

                    cancel = sec_dev.find_one(os.path.join('update',
                                                  'cancel'))
                    if not cancel:
                        cancel = sec_dev.find_one('cancel')
                    cancel.value = 1
                    stat, mesg = 1, 'Interrupted'

    if stat and mesg == 'flash_wearout':
        mesg = ('Secure update is delayed due to excessive flash counts.\n'
                'Please wait 30 seconds and try again.')

    if stat:
        LOG.error(mesg)
    else:
        LOG.info(mesg)

    total = datetime.now() - start
    total_str = str(total)[:-4] # limit to hundredths
    LOG.info('Total time: %s', total_str)
    sys.exit(stat)


if __name__ == '__main__':
    main()
