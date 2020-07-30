#! /usr/bin/env python3
# Copyright(c) 2019-2020, Intel Corporation
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
from datetime import datetime, timedelta
import signal
import errno
import logging
import tempfile
import shutil
from opae.admin.fpga import fpga
from opae.admin.utils.progress import progress
from opae.admin.version import pretty_version

if sys.version_info[0] == 2:
    input = raw_input  # noqa pylint: disable=E0602

    def uuid_from_bytes(blob):
        return uuid.UUID(bytes=[b for b in reversed(blob)])
else:
    def uuid_from_bytes(blob):
        return uuid.UUID(bytes=blob)

DEFAULT_BDF = 'ssss:bb:dd.f'

VALID_GBS_GUID = uuid.UUID('31303076-5342-47b7-4147-50466e6f6558')

BLOCK0_TYPE_STATIC_REGION = 0
BLOCK0_TYPE_BMC = 1
BLOCK0_TYPE_GBS = 2

BLOCK0_SUBTYPE_UPDATE = 0x0000
BLOCK0_SUBTYPE_CANCELLATION = 0x0100
BLOCK0_SUBTYPE_ROOT_KEY_HASH_256 = 0x0200
BLOCK0_SUBTYPE_ROOT_KEY_HASH_384 = 0x0300

BLOCK0_CONTYPE_MASK = 0x00ff
BLOCK0_CONSUBTYPE_MASK = 0xff00

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
                        version='%(prog)s {}'.format(pretty_version()),
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

    metadata = infile.read(metadata_length)
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
        output = subprocess.check_output(conf_args)
        output = output.decode(sys.getdefaultencoding())
    except subprocess.CalledProcessError as exc:
        return (exc.returncode,
                exc.output.decode(sys.getdefaultencoding()) +
                '\nPartial Reconfiguration failed')

    return (0, output + '\nPartial Reconfiguration OK')


def decode_auth_block0(infile, prompt):
    """Reads and decodes an authentication block 0.

    infile - a valid, open file object specified on the command line.

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
        elif len(ans) == 0 or ans.lower().startswith('n'):
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
    else:
        regex = re.compile(pcie_addr_pattern, re.IGNORECASE)
        match = regex.match(bdf)
        if match:
            return bdf

    return None


class SecureUpdateError(Exception):
    """Secure update exception"""
    def __init__(self, arg):
        super(SecureUpdateError, self).__init__(self)
        self.errno, self.strerror = arg


def update_fw(args, pac):
    """Writes firmware to secure device.

    args - the object resulting from command-line parsing.

    returns a 2-tuple of the process exit status and a message.
    """
    # bytes/sec when staging is flash
    flash_copy_bps = 43000.0
    # bytes/sec when staging area is dram
    dram_copy_bps = 92000.0
    dram_copy_offset = 42.0

    infile = args.file

    orig_pos = infile.tell()
    infile.seek(0, os.SEEK_END)
    payload_size = infile.tell() - orig_pos
    infile.close()

    sec_dev = pac.secure_dev
    error = sec_dev.find_one(os.path.join('update', 'error'))
    filename = sec_dev.find_one(os.path.join('update', 'filename'))
    size = sec_dev.find_one(os.path.join('update', 'remaining_size'))
    status = sec_dev.find_one(os.path.join('update', 'status'))

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
            os.remove(tfile.name)
            return errno.ETIMEDOUT, 'Secure update timed out'

    LOG.info('updating from file %s with size %d',
             infile.name, payload_size)

    filename.value = os.path.join('intel', os.path.basename(tfile.name))

    retries = 0
    timeout = 1.0
    max_retries = 60 * 5
    while status.value == 'read_file' or status.value == 'preparing':
        time.sleep(timeout)
        retries += 1
        if retries >= max_retries:
            os.remove(tfile.name)
            return errno.ETIMEDOUT, 'Secure update timed out'

    if status.value == 'idle':
        e = error.value
        if e:
            os.remove(tfile.name)
            return 1, e

    progress_cfg = {}
    level = min([handler.level for handler in LOG.handlers])
    if level < logging.INFO:
        progress_cfg['log'] = LOG.debug
    else:
        progress_cfg['stream'] = sys.stdout

    retries = 0
    timeout = 1.0
    max_retries = 60 * 60 * 2
    with progress(bytes=payload_size, **progress_cfg) as prg:
        while int(size.value) > 0:
            time.sleep(timeout)
            retries += 1
            if retries >= max_retries:
                os.remove(tfile.name)
                return errno.ETIMEDOUT, 'Secure update timed out'
            prg.update(payload_size - int(size.value))

    if status.value == 'idle':
        e = error.value
        if e:
            os.remove(tfile.name)
            return 1, e

    if pac.fme.have_node('tcm'):
        estimated_time = payload_size / dram_copy_bps + dram_copy_offset
    else:
        estimated_time = payload_size / flash_copy_bps
    # over-estimate by 1.5 to account for flash performance degradation
    estimated_time *= 1.5

    LOG.info('applying update to %s', pac.pci_node.pci_address)

    retries = 0
    timeout = 1.0
    max_retries = 60 * 60 * 3
    with progress(time=estimated_time, **progress_cfg) as prg:
        while status.value == 'writing' or status.value == 'programming':
            time.sleep(timeout)
            retries += 1
            if retries >= max_retries:
                os.remove(tfile.name)
                return errno.ETIMEDOUT, 'Secure update timed out'
            prg.tick()

    if status.value == 'idle':
        e = error.value
        if e:
            os.remove(tfile.name)
            return 1, e

    LOG.info('update of %s complete', pac.pci_node.pci_address)

    os.remove(tfile.name)

    return 0, 'Secure update OK'


def sig_handler(signum, frame):
    """raise exception for SIGTERM
    """
    raise SecureUpdateError((1, 'Caught SIGTERM'))


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
    log_hndlr.setFormatter(logging.Formatter(log_fmt))

    log_hndlr.setLevel(LOG_NAMES_TO_LEVELS[args.log_level])

    LOG.addHandler(log_hndlr)

    signal.signal(signal.SIGTERM, sig_handler)

    LOG.debug('fw file: %s', args.file.name)
    LOG.debug('addr: %s', args.bdf)

    stat = 1
    mesg = 'Secure update failed'
    gbs_hdr = None

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

    LOG.warning('Update starting. Please do not interrupt.')

    start = datetime.now()

    if gbs_hdr is not None:
        args.file.close()
        stat, mesg = do_partial_reconf(pac.pci_node.pci_address,
                                       args.file.name)
    elif blk0 is not None:
        sec_dev = pac.secure_dev
        if not sec_dev:
            LOG.error('Failed to find secure '
                      'device for PAC %s', pac.pci_node.pci_address)
            sys.exit(1)

        LOG.debug('Found secure device for PAC '
                  '%s', pac.pci_node.pci_address)

        try:
            stat, mesg = update_fw(args, pac)
        except SecureUpdateError as exc:
            stat, mesg = exc.errno, exc.strerror
        except KeyboardInterrupt:
            if TMPFILE and os.path.isfile(TMPFILE):
                os.remove(TMPFILE)
            cancel = sec_dev.find_one(os.path.join('update', 'cancel'))
            cancel.value = 1
            stat, mesg = 1, 'Interrupted'

    if stat and mesg == 'flash_wearout':
        mesg = ('Secure update is delayed due to excessive flash counts.\n'
                'Please wait 30 seconds and try again.')

    if stat:
        LOG.error(mesg)
    else:
        LOG.info(mesg)

    LOG.info('Total time: %s', datetime.now() - start)
    sys.exit(stat)


if __name__ == '__main__':
    main()
