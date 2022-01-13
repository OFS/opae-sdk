#!/usr/bin/env python3

from pathlib import Path
import subprocess as sp
import argparse
import logging
import tarfile
import shlex
import glob
import sys
import re
import io
import os

# list of paths to include in mock
mock_paths = [
    '/dev/dfl-*',
    '/sys/bus/pci/devices/{bdf}',
    '/sys/class/fpga_region',
    '/sys/class/uio',
]

class CommandError(Exception):
    '''Exception raised by run_command() upon non-zero return value from executed command'''

    def __init__(self, command, p):
        self.command = ' '.join(command)
        if len(p.stderr) != 0:
            self.error = p.stderr
        elif len(p.stdout) != 0:
            self.error = p.stdout
        else:
            self.error = 'unknown'
        self.output = p.stdout

    def __str__(self):
        return 'Command failed: {}: {}'.format(self.command, self.error)

def run_command(cmd, env=None, timeout_secs=60, ignore_error=False):
    '''Helper function to execute command and return stripped output'''

    cmd = [str(c) for c in cmd]
    p = sp.run(cmd, env=env, timeout=timeout_secs, encoding='UTF-8', stdout=sp.PIPE, stderr=sp.PIPE)

    if p.returncode != 0 and not ignore_error:
        raise CommandError(cmd, p)
    elif p.returncode != 0:
        return ''

    return p.stdout.strip()

def find_bdf(vid, did):
    '''Lookup PCI device from vendor and device id, but fail if anything but one device is found'''

    def parse_line(line):
        '''Helper to output line from lspci command'''

        parser = argparse.ArgumentParser()
        parser.add_argument('bdf')
        parser.add_argument('class')
        parser.add_argument('vendor')
        parser.add_argument('device')
        parser.add_argument('subsystem_vendor')
        parser.add_argument('subsystem_device')
        parser.add_argument('-r', nargs='?', dest='revision')
        parser.add_argument('-p', nargs='?', dest='progif')

        # lspci prints quoted fields separated by spaces, so parse that using shlex
        tokens = shlex.split(line)

        return parser.parse_args(tokens)

    # look up devices with lspci and parse the output
    out = run_command(['lspci', '-mm', '-D', '-d', f'{vid}:{did}'])
    devices = [parse_line(line) for line in out.splitlines()]

    if not devices:
        logging.error(f'No PCI devices found with id {vid}:{did}')
        return None

    # bail out if multiple physical devices are found (but filter out multiple functions
    # of the same device)
    if len(devices) > 1 and len(set([d.bdf[:-1] for d in devices])) > 1:
        logging.error(f'Multiple PCI devices found with id {vid}:{did}')
        for device in devices:
            logging.info(' {bdf} {class}: {vendor} {device} (rev {revision})'.format(**vars(device)))
        return None

    return devices[0].bdf

def get_info(tar, path):
    '''Get tar info for passed path; update user/group with default names, and add 'tmp' prefix to path'''

    info = tar.gettarinfo(path)
    info.uid = 1000
    info.gid = 1000
    info.uname = 'user'
    info.gname = 'user'
    info.name = 'tmp' + str(path)

    if path.is_dir() or path.is_symlink():
        info.mode = 0o777
    else:
        info.mode = 0o666

    return info

def add_dir(tar, path):
    '''add directory entry to mock'''

    if path.is_symlink() and path.name in ('subsystem', 'driver', 'firmware_node'):
        logging.warning(f'skipping dangling symlink: {path}')
        return

    logging.info(f'adding dir: {path}')

    info = get_info(tar, path)
    tar.addfile(info)

def add_file(tar, path):
    '''add file/symlink entry to tar while handling weird files gracefully'''

    if path.is_symlink() and path.name in ('subsystem', 'driver', 'firmware_node'):
        logging.warning(f'skipping dangling symlink: {path}')
        return

    logging.info(f'adding file: {path}')

    if not path.is_symlink():
        assert not path.is_dir(), f'Directory passed to add_file(): {path}'

    info = get_info(tar, path)

    # add symlink as they are
    if path.is_symlink():
        tar.addfile(info)
        return

    # convert special files to regular files
    if not path.is_file():
        info.type = tarfile.REGTYPE

    try:
        b = path.read_bytes()
    except OSError:
        # some sysfs files are unreadable, so create those as zero filled files
        b = b'\x00' * 1024

    f = io.BytesIO(b)
    info.size = len(b)
    tar.addfile(info, f)

def add_tree(tar, path):
    '''add an entire tree to the mock'''

    logging.info(f'adding tree: {path}')

    if path.is_symlink():
        # if the passed path is a symlink then add it to the mock, and
        # continue from its target
        add_file(tar, path)
        path = Path(os.path.realpath(path))

    # also add the passed path to the mock
    if path.is_dir():
        add_dir(tar, path)
    else:
        add_file(tar, path)

    # walk the tree
    for root,dirs,files in os.walk(path):
        root = Path(root)

        for name in dirs:
            add_dir(tar, root / name)

        for name in files:
            add_file(tar, root / name)

def create_mock(bdf, output):
    '''open the mock tar file, and add the configured trees to it'''

    with tarfile.open(output, 'w|gz') as tar:
        for path in mock_paths:
            for p in glob.glob(path.format(bdf=bdf)):
                add_tree(tar, Path(p))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Create mock tar.gz of dfl sysfs tree')
    parser.add_argument('--log-level', default='info', choices=('debug', 'info', 'warning', 'error'), help='log level to configure')
    parser.add_argument('--vid', default='0x8086', help='Expected PCI Vendor ID for device')
    parser.add_argument('--did', default='0xbcce', help='Expected PCI Device ID for device')
    parser.add_argument('--bdf', help='Override/specify <bus>:<device>:<function> - e.g. 0000:01:00.0')
    parser.add_argument('output', metavar='OUTPUT', help='file to write mock archive to')

    args = parser.parse_args()

    logging.basicConfig(level=args.log_level.upper())

    # look up a compatible device if --bdf isn't specified
    bdf = args.bdf or find_bdf(args.vid, args.did)
    if bdf is None:
        logging.error('Please provide the --bdf argument')
        sys.exit(1)

    # verify specified/found --bdf
    match = re.match(r'(?:([0-9A-Fa-f]{4}):)?([0-9A-Fa-f]{2}):([0-9A-Fa-f]{2})\.([0-9A-Fa-f])', bdf)
    if not match:
        logging.error(f'Invalid --bdf argument: \'{bdf}\'')
        sys.exit(1)

    # prepend bdf domain if needed
    (domain,bus,device,function) = match.groups()
    bdf = f'{domain or "0000"}:{bus}:{device}.*'

    # verify that specified --bdf matches a device
    if not glob.glob(f'/sys/bus/pci/devices/{bdf}'):
        logging.error(f'No PCI device for --bdf argument: \'{bdf}\'')
        sys.exit(1)

    create_mock(bdf, args.output)
