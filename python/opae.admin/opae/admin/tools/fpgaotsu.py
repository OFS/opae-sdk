#! /usr/bin/env python3
# Copyright(c) 2019, Intel Corporation
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

"""Perform One-Time Secure Update for PAC devices."""

from __future__ import absolute_import
import argparse
import sys
import logging
import json
import os
import tempfile
import filecmp
import errno
import signal
import threading
from datetime import datetime
from opae.admin.fpga import fpga
from opae.admin.utils.mtd import mtd
from opae.admin.utils import version_comparator
from opae.admin.version import pretty_version

LOG = logging.getLogger()

if sys.version_info[0] == 3:
    str_type = str
else:
    str_type = basestring  # noqa pylint: disable=E0602


def to_int(value):
    """Can value be converted to an integer?

       Args:
           value: The value to consider.

       Returns:
           A 2-tuple of a boolean indicating the status,
           and the converted number or (False, -1) if
           conversion is not possible.
    """
    if isinstance(value, str_type):
        try:
            res = int(value, 0)
        except ValueError:
            return (False, -1)
        return (True, res)

    if isinstance(value, int):
        return (True, value)

    return (False, -1)


def all_or_none(obj, *keys):
    """Determine if all keys present or absent in obj."""
    values = [obj.get(key) for key in keys]
    if any(values) and not all(values):
        pairs = list(zip(keys, values))
        present = [k for k, v in pairs if v is not None]
        missing = [k for k, v in pairs if v is None]
        raise KeyError('"flash" key "%s" requires "%s"' %
                       (str(present), str(missing)))


class otsu_manifest_loader(object):
    """Loads an fpgaotsu manifest """

    MANIFEST_VERSION = 2

    REQUIRES_LABELS = ['max10_version',
                       'bmcfw_version',
                       'fpga_image_load',
                       'bmc_aux_fw_rev']

    def __init__(self, fp):
        self._fp = fp
        self._json_obj = None

    def validate_mandatory_keys(self, obj):
        """Verify that mandatory keys are present.

        Verify format and content.
        """
        mandatory_keys = ['product', 'vendor', 'device',
                          'program', 'flash', 'version']
        for key in mandatory_keys:
            if obj.get(key) is None:
                raise KeyError('"%s" key not found in manifest' % (key))

        hex_keys = ['vendor', 'device']
        for key in hex_keys:
            if not to_int(obj[key])[0]:
                raise TypeError('"%s" key not in integer format: %s' %
                                (key, obj[key]))

        if obj['program'] != 'one-time-update':
            raise ValueError('expected one-time-update for "program" key '
                             'but received: %s' % (obj['program']))

        res = to_int(obj['version'])
        if not res[0]:
            raise TypeError('"version" key not in integer format: %s' %
                            (obj['version']))
        if res[1] != self.MANIFEST_VERSION:
            raise ValueError('expected %d for "version" key '
                             'but found: %s' % (self.MANIFEST_VERSION, res[1]))

    def validate_requires_section(self, obj):
        """Verify an optional "requires" array, when present."""
        requires = obj.get('requires')
        if not requires:
            return
        if not isinstance(requires, list):
            raise TypeError('"requires" key must be a list of expressions')
        for req in requires:
            if not isinstance(req, str_type):
                raise TypeError('"requires" entry not a string: %s' % (req))
            comp = version_comparator(req)
            if not comp.parse():
                raise ValueError('invalid "requires" expression: %s' % (req))
            if comp.label not in self.REQUIRES_LABELS:
                raise ValueError('%s is not a valid "requires" label' %
                                 (comp.label))

    def validate_flash_section(self, obj, directory):
        """Verify a single "flash" object from the manifest."""
        mandatory_keys = ['type']
        for key in mandatory_keys:
            if obj.get(key) is None:
                raise KeyError('"flash" section missing "%s" key' % (key))

        # If either "filename" or "start" exists,
        # then they must both exist.
        filename = obj.get('filename')
        start = obj.get('start')
        all_or_none(obj, 'filename', 'start')

        # If either "erase_start" or "erase_end" exists,
        # then they must both exist.
        erase_start = obj.get('erase_start')
        erase_end = obj.get('erase_end')
        all_or_none(obj, 'erase_start', 'erase_end')

        if erase_start:
            if not to_int(erase_start)[0]:
                raise TypeError('"flash" key "erase_start" '
                                'not in integer format: %s' %
                                (erase_start))
            if not to_int(erase_end)[0]:
                raise TypeError('"flash" key "erase_end" '
                                'not in integer format: %s' %
                                (erase_end))

        if filename:
            # "start" must be an integer.
            if not to_int(start)[0]:
                raise TypeError('"flash" key "start" not in '
                                'integer format: %s' % (start))

            # "end" and "seek" must be integers, if present.
            optional_keys = ['end', 'seek']
            for i, key in enumerate(optional_keys):
                if obj.get(key) and not to_int(obj[key])[0]:
                    raise TypeError('"flash" key "%s" not in '
                                    'integer format: %s' %
                                    (optional_keys[i], obj[key]))

            boolean_keys = ['verify']
            for i, key in enumerate(boolean_keys):
                if obj.get(key) and not isinstance(obj[key], bool):
                    raise TypeError('"flash" key "%s" not boolean: %s' %
                                    (boolean_keys[i], obj[key]))

            # The file given for "filename" must exist.
            flash_file = os.path.join(directory, filename)
            if not os.path.isfile(flash_file):
                raise OSError(errno.ENOENT, '"%s" not found' % (flash_file))

            seek = to_int(obj.get('seek', 0))[1]
            stat_info = os.stat(flash_file)

            if obj.get('end') is None:
                begin = to_int(start)[1]
                obj['end'] = '0x%x' % (begin + (stat_info.st_size - seek) - 1)

            size = (to_int(obj['end'])[1] + 1) - to_int(obj['start'])[1]
            if seek + size > stat_info.st_size:
                raise ValueError('seek + size > file_size : '
                                 '0x%x + 0x%x > 0x%x' %
                                 (seek, size, stat_info.st_size))

        for nested in obj.get('flash', []):
            self.validate_flash_section(nested, directory)

    def load_and_validate(self):
        """Load and Verify contents

           Verify that the required keys are present.
           Verify that all "filename" keys correspond to files
           that are present on the system.
           Verify that all address fields are hex numbers.
        """
        try:
            obj = json.load(self._fp)
        except ValueError:
            msg = 'Invalid JSON format in {}'.format(self._fp.name)
            LOG.exception(msg)
            return None

        try:
            self.validate_mandatory_keys(obj)
            self.validate_requires_section(obj)
        except (KeyError, TypeError, ValueError) as exc:
            LOG.exception(exc)
            return None

        directory = os.path.dirname(self._fp.name)
        for item in obj['flash']:
            try:
                self.validate_flash_section(item, directory)
            except (KeyError, TypeError, OSError, ValueError) as flsh_exc:
                LOG.exception(flsh_exc)
                return None

        return obj

    def load(self):
        """Load JSON config"""
        if self._json_obj is None:
            self._json_obj = self.load_and_validate()
        return self._json_obj


class otsu_updater(object):
    """Applies One-Time Secure Update per the given manifest"""
    def __init__(self, fw_dir, pac, config, chunk_size=64*1024):
        self._fw_dir = fw_dir
        self._pac = pac
        self._config = config
        self._chunk_size = chunk_size
        self._timeout = 8*3600.0
        self._thread = None
        self._errors = []

    @property
    def error_count(self):
        """Return the current number of errors."""
        return len(self._errors)

    def error(self, msg):
        msg = '{} {}: {}'.format(self._config['product'],
                                 self._pac.pci_node.pci_address,
                                 msg)
        self._errors.append(msg)

    def log_errors(self, logfn):
        for error in self._errors:
            logfn(error)

    @property
    def pac(self):
        """Retrieve the PAC associated with this updater"""
        return self._pac

    def check_requires(self):
        """Verify the (optional) "requires" expressions."""
        requires = self._config.get('requires')
        if not requires:
            return True
        for req in requires:
            comp = version_comparator(req)
            comp.parse()
            version = getattr(self._pac.fme, comp.label)
            if not comp.compare(str(version)):
                LOG.warning('"requires" expression %s failed' % (req))
                return False
        return True

    def erase(self, obj, mtd_dev):
        """Erase the flash range described in obj.

        Args:
            obj: an object for one of the parsed "flash" sections
                from the manifest.
            mtd_dev: an mtd object for the open flash device.
        """
        if obj.get('erase_start') is not None:
            erase_start = to_int(obj['erase_start'])[1]
            erase_end = to_int(obj['erase_end'])[1]
            LOG.info('Erasing %s@0x%x for %d bytes',
                     obj['type'], erase_start, (erase_end + 1) - erase_start)
            mtd_dev.erase(erase_start,
                          (erase_end + 1) - erase_start)

    def read_modify_write(self, obj, mtd_dev):
        """read modify write the flash range described in obj.

        Args:
            obj: an object for one of the parsed "flash" sections
                from the manifest.
            mtd_dev: an mtd object for the open flash device.
        """
        if obj.get('filename') is not None:
            seek = 0 if obj.get('seek') is None else to_int(obj['seek'])[1]
            start = to_int(obj['start'])[1]
            end = to_int(obj['end'])[1]
            erase_start = to_int(obj['erase_start'])[1]
            erase_end = to_int(obj['erase_end'])[1]
            filename = os.path.join(self._fw_dir, obj['filename'])
            verify = obj.get('verify', False)

            with open(filename, 'rb') as infile:
                status = self.verify(obj, mtd_dev, infile, report_fail=False)
                if status:
                    LOG.info('Flash content matches with file %s',
                             os.path.basename(filename))
                    return True

                LOG.info('Read/Modify/Writing %s@0x%x for %d bytes (%s)',
                         obj['type'], start, (end + 1) - start,
                         os.path.basename(filename))

                if infile.tell() != seek:
                    raise IOError('failed to seek in input file %s: 0x%x' %
                                  (os.path.basename(filename), seek))
                data = infile.read()
                if not data:
                    raise IOError('failed to read file %s' %
                                  (os.path.basename(filename)))
                mtd_dev.replace(data,
                                (erase_end + 1) - erase_start,
                                start)

                if verify:
                    return self.verify(obj, mtd_dev, infile)

        return True

    def write(self, obj, mtd_dev):
        """Write the flash range described in obj.

        Args:
            obj: an object for one of the parsed "flash" sections
                from the manifest.
            mtd_dev: an mtd object for the open flash device.
        """
        if obj.get('filename') is not None:
            seek = 0 if obj.get('seek') is None else to_int(obj['seek'])[1]
            start = to_int(obj['start'])[1]
            end = to_int(obj['end'])[1]
            filename = os.path.join(self._fw_dir, obj['filename'])
            verify = obj.get('verify', False)

            with open(filename, 'rb') as infile:
                LOG.info('Writing %s@0x%x for %d bytes (%s)',
                         obj['type'], start, (end + 1) - start,
                         os.path.basename(filename))

                infile.seek(seek)
                if infile.tell() != seek:
                    raise IOError('failed to seek in input file %s: 0x%x' %
                                  (filename, seek))

                if min([l.level for l in LOG.handlers]) < logging.INFO:
                    prog = LOG.debug
                else:
                    prog = sys.stdout

                mtd_dev.copy_from(infile,
                                  (end + 1) - start,
                                  start,
                                  progress=prog,
                                  chunked=self._chunk_size)

                if verify:
                    return self.verify(obj, mtd_dev, infile)

        return True

    def verify(self, obj, mtd_dev, infile, report_fail=True):
        """Verify the flash range described in obj.

        Args:
            obj: an object for one of the parsed "flash" sections
                from the manifest.
            mtd_dev: an mtd object for the open flash device.
            infile: an open file object to the source file content.
        """
        seek = 0 if obj.get('seek') is None else to_int(obj['seek'])[1]
        start = to_int(obj['start'])[1]
        end = to_int(obj['end'])[1]
        filename = os.path.join(self._fw_dir, obj['filename'])

        infile.seek(seek)
        if infile.tell() != seek:
            raise IOError('failed to seek in input file %s: 0x%x' %
                          (filename, seek))

        src_bits = tempfile.NamedTemporaryFile(mode='wb', delete=False)
        src_bits.write(infile.read((end + 1) - start))
        src_bits.close()

        flash_bits = tempfile.NamedTemporaryFile(mode='wb', delete=False)
        LOG.info('Reading %s@0x%x for %d bytes for verification',
                 obj['type'], start, (end + 1) - start)

        if min([l.level for l in LOG.handlers]) < logging.INFO:
            prog = LOG.debug
        else:
            prog = sys.stdout

        mtd_dev.copy_to(flash_bits,
                        (end + 1) - start,
                        start,
                        progress=prog,
                        chunked=self._chunk_size)
        flash_bits.close()

        compare = filecmp.cmp(src_bits.name, flash_bits.name, shallow=False)

        os.remove(src_bits.name)
        os.remove(flash_bits.name)

        if compare:
            LOG.info('Verified %s@0x%x for %d bytes (%s)',
                     obj['type'], start, (end + 1) - start,
                     os.path.basename(filename))
            return True

        if report_fail:
            msg = 'Verification of {} @0x{} failed'.format(
                  os.path.basename(filename), start)
            self.error(msg)

        return False

    def wait(self):
        """Wait for this updater's thread to terminate

        Returns:
            0 if the thread completed within the given timeout.
            non-zero if thread timed out or if the update failed.
        """
        self._thread.join(timeout=self._timeout)
        if self._thread.isAlive():
            self.error('update timed out')
        return self.error_count

    def update(self):
        """Begin this update in a separate thread"""
        self._thread = threading.Thread(target=self._update,
                                        name=self._pac.pci_node.pci_address)
        self._thread.start()

    def _update(self):
        """Apply the update described by each 'flash' entry."""

        LOG.info('Updating %s : %s',
                 self._config['product'],
                 self._pac.pci_node.pci_address)

        controls = self._pac.fme.flash_controls()

        for flash in self._config['flash']:
            if not self.process_flash_item(controls, flash):
                return False

        return self.error_count == 0

    def process_flash_item(self, controls, flash):
        # Find the flash_control object that matches the
        # 'type' encoded in the flash entry.
        ctrls = [c for c in controls if c.name == flash['type']]

        if not ctrls:
            msg = 'flash type "{}" not found'.format(flash['type'])
            self.error(msg)
            raise AttributeError(msg)

        with ctrls[0] as ctrl:
            with mtd(ctrl.devpath).open('r+b') as mtd_dev:
                try:
                    if flash.get('read-modify-write', False):
                        self.read_modify_write(flash, mtd_dev)
                    else:
                        self.erase(flash, mtd_dev)
                        self.write(flash, mtd_dev)
                except IOError as eexc:
                    LOG.exception(eexc)
                    self.error(eexc)
                    return False

            for nested in flash.get('flash', []):
                if not self.process_flash_item(controls, nested):
                    return False

        return self.error_count == 0


def parse_args():
    """Parses command line arguments
    """
    parser = argparse.ArgumentParser()

    manifest_help = 'The configuration file describing ' \
                    'the One-Time Secure Update'
    parser.add_argument('manifest', type=argparse.FileType('r'),
                        nargs='?', help=manifest_help)

    log_levels = ['debug', 'info', 'warning', 'error', 'critical']
    parser.add_argument('--log-level', choices=log_levels,
                        default='info', help='log level to use')
    parser.add_argument('--verify', action='store_true', default=False,
                        help='verify whether PACs need updating and exit')
    parser.add_argument('--rsu', action='store_true', default=False,
                        help='perform "RSU" operation after update')
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s {}'.format(pretty_version()),
                        help='display version information and exit')

    return parser, parser.parse_args()


def sig_handler(signum, frame):
    """Registered signals become KeyboardInterrupt."""
    raise KeyboardInterrupt('interrupt signal received')


def run_updaters(updaters):
    """Run each of the updates found in updaters

    Args:
        updaters: a list of otsu_updater objects.

    Returns:
        0 on success.
        The non-zero number of errors on failure.
    """

    for updater in updaters:
        try:
            updater.update()
        except (IOError, AttributeError, KeyboardInterrupt) as exc:
            LOG.exception(exc)
            updater.error(exc)

    errors = 0
    for updater in updaters:
        errors += updater.wait()

    return errors


def main():
    """The main entry point"""
    parser, args = parse_args()

    if args.manifest is None:
        print('Error: manifest is a required argument\n')
        parser.print_help(sys.stderr)
        sys.exit(1)

    LOG.setLevel(logging.NOTSET)
    log_fmt = ('[%(asctime)-15s] [%(levelname)-8s] '
               '[%(threadName)s] %(message)s')
    log_hndlr = logging.StreamHandler(sys.stdout)
    log_hndlr.setFormatter(logging.Formatter(log_fmt))

    log_hndlr.setLevel(logging.getLevelName(args.log_level.upper()))

    LOG.addHandler(log_hndlr)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    loader = otsu_manifest_loader(args.manifest)
    json_cfg = loader.load()
    if json_cfg is None:
        LOG.error('Failed to process manifest: %s', args.manifest.name)
        sys.exit(1)

    # Find the PAC described by the manifest.
    enum_filter = {'pci_node.vendor_id': to_int(json_cfg['vendor'])[1],
                   'pci_node.device_id': to_int(json_cfg['device'])[1]}

    pacs = fpga.enum([enum_filter])
    if not pacs:
        LOG.error('failed to find any suitable PAC: %s:%s',
                  json_cfg['vendor'], json_cfg['device'])
        sys.exit(1)

    errors = 0
    updaters = []
    for pac in pacs:
        if pac.secure_dev:
            LOG.info('%s %s is already secure.', json_cfg['product'],
                     pac.pci_node.pci_address)
            continue

        LOG.info('%s %s is not secure.', json_cfg['product'],
                 pac.pci_node.pci_address)

        updater = otsu_updater(os.path.dirname(args.manifest.name),
                               pac,
                               json_cfg)
        if updater.check_requires():
            updaters.append(updater)
        else:
            errors += 1
            LOG.warning('%s %s one or more prerequisite checks '
                        'failed. Skipping this device.',
                        json_cfg['product'],
                        pac.pci_node.pci_address)

    LOG.warning('Update starting. Please do not interrupt.')

    start = datetime.now()

    if not args.verify:
        errors += run_updaters(updaters)
    else:
        errors += len(updaters)

    if args.rsu:
        for updater in updaters:
            if updater.error_count == 0:
                updater.pac.safe_rsu_boot()

    LOG.info('Total time: %s', datetime.now() - start)

    if errors > 0:
        for updater in updaters:
            updater.log_errors(LOG.error)
        LOG.error('One-Time Secure Update failed')
    else:
        LOG.info('One-Time Secure Update OK')
    sys.exit(errors)


if __name__ == '__main__':
    main()
