#! /usr/bin/env python3
#  Copyright(c) 2020, Intel Corporation
#
#  Redistribution  and  use  in source  and  binary  forms,  with  or  without
#  modification, are permitted provided that the following conditions are met:
#
#  * Redistributions of  source code  must retain the  above copyright notice,
#    this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#  * Neither the name  of Intel Corporation  nor the names of its contributors
#    may be used to  endorse or promote  products derived  from this  software
#    without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
#  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
#  LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
#  CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
#  SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
#  INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
#  CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
# pylint: disable=E0001
import argparse
import json
import logging
import os
import re
import subprocess
import sys

from pathlib import Path


def get_config(cfgname, cfgfile):
    cfg = {'default': {'build_dir': 'default'}}
    if not Path(cfgfile).exists():
        logging.warning(f'{cfgfile} not found')
    else:
        with open(cfgfile, 'r') as fp:
            cfg.update(json.load(fp))
    user_cfgfile = Path('~/.build.json').expanduser()
    if user_cfgfile.exists():
        with user_cfgfile.open('r') as fp:
            cfg.update(json.load(fp))

    if cfgname not in cfg:
        raise SystemExit(f'{cfgname} not defined')
    build_cfg = cfg[cfgname]
    if 'from' in build_cfg:
        cfgbase = build_cfg['from']
        if cfgbase not in cfg:
            raise SystemExit(f'from({cfgbase}) not defined')
        from_cfg = cfg[cfgbase]
        from_cfg.update(build_cfg)
        build_cfg = from_cfg

    return build_cfg


def run(cmd, **kwargs):
    logging.info(' '.join(cmd))
    subprocess.run(cmd, **kwargs)


def configure(cfgname,
              cfgfile='.build.json', stdout=sys.stdout, stderr=sys.stderr):
    root = Path.cwd()
    cfg = get_config(cfgname, cfgfile)
    version = '{}.{}'.format(*sys.version_info[:2])
    cfg.setdefault('defs', {})['OPAE_PYTHON_VERSION'] = version
    build_dir = Path('.build', cfg.get('build_dir', cfgname))
    cmd = ['cmake', str(root)]
    if 'build_type' in cfg:
        cmd.append(f'-DCMAKE_BUILD_TYPE={cfg["build_type"]}')
    for k, v in cfg.get('defs', {}).items():
        cmd.append(f'-D{k}={v}')
    if not build_dir.exists():
        build_dir.mkdir(parents=True)
    else:
        logging.warning(f'{build_dir} already exists')
    run(cmd, cwd=str(build_dir), stdout=stdout, stderr=stderr)
    return build_dir


def buildit(build_dir, targets=[], stdout=sys.stdout, stderr=sys.stderr):
    nproc = str(os.cpu_count())
    build_cmd = ['make'] + targets
    try:
        run(build_cmd + ['-j', nproc],
            cwd=str(build_dir), stdout=stdout, stderr=stderr)
    except subprocess.CalledProcessError as err:
        logging.error(err)
        run(build_cmd, cwd=str(build_dir), stdout=stdout, stderr=stderr)


def smart_targets(build_dir, files_in):
    with Path(build_dir, 'compile_commands.json').open('r') as fp:
        ccmds = json.load(fp)
    compile_targets = {}
    for cmd in ccmds:
        cmd_dir = Path(cmd['directory'])
        if cmd_dir.joinpath('Makefile').exists():
            compile_targets[cmd['file']] = cmd_dir.stem
    common = set(files_in).intersection(set(compile_targets.keys()))
    targets = [compile_targets[f] for f in common]
    match_depends(build_dir, files_in, targets)
    return list(set(targets))


TARGET_RE = re.compile('(?P<target>.*?): (?P<file>.*)')


def match_depends(build_dir, files_in, targets):
    for root, dirs, files in os.walk(build_dir):
        if 'depend.make' in files:
            root_dir = Path(root)
            target_name = str(root_dir.stem).rstrip('.dir')
            if target_name in targets:
                continue
            with Path(root, 'depend.make').open('r') as fp:
                lines = [ln for ln in fp.readlines() if not ln.startswith('#')]
            for ln in lines:
                m = TARGET_RE.match(ln.strip())
                if m:
                    dep_file = m.group('file')
                    if Path(dep_file).is_absolute():
                        continue
                    abs_f = Path(build_dir, m.group('file')).resolve()
                    if str(abs_f) in files_in:
                        targets.append(target_name)


def clean(build_dir):
    run(['make', 'clean'], cwd=build_dir)


def main(args=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('cfgname', nargs='?',
                        help='name of configuration')
    parser.add_argument('-c', '--cfgfile', default='.build.json',
                        help='path to configuration file')
    parser.add_argument('-t', '--targets', nargs='*', default=[],
                        help='list of targets to build')
    parser.add_argument('--output', type=argparse.FileType('w'),
                        help='redirect stdout and stderr to file')
    parser.add_argument('--smart-targets', nargs='*', default=[],
                        help='build targets that depend on files')
    parser.add_argument('--git-modified', action='store_true', default=False,
                        help='build targets affected by git modified files')
    parser.add_argument('--clean', action='store_true', default=False,
                        help='run make clean before building')
    args = parser.parse_args(args=args)

    logging.basicConfig(level=logging.NOTSET)

    cfgname = args.cfgname or 'default'

    stdout = args.output or sys.stdout
    stderr = args.output or sys.stderr
    build_dir = configure(cfgname, args.cfgfile, stdout=stdout, stderr=stderr)

    if args.git_modified:
        output = subprocess.check_output(['git', 'ls-files', '-m']).strip()
        if output:
            args.smart_targets.extend(output.decode().split('\n'))

    if args.smart_targets:
        abs_files = [os.path.abspath(p) for p in args.smart_targets]
        targets = smart_targets(build_dir, abs_files)
    else:
        targets = args.targets

    if args.clean:
        clean(build_dir)

    buildit(build_dir, targets=targets, stdout=stdout, stderr=stderr)


if __name__ == '__main__':
    main()
