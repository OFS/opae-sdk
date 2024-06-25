#!/usr/bin/env python3
# Copyright(c) 2022-2023, Intel Corporation
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

import argparse
import os
import re
import subprocess
import sys

try:
    from pathlib import Path, PurePath
except ImportError:
    from pathlib2 import Path, PurePath


RPM_SCANNING = 1
RPM_COLLECTING = 2

RPM_STATE_TO_NAMES = {
  RPM_SCANNING: 'SCANNING',
  RPM_COLLECTING: 'COLLECTING'
}

RPM_SECTION = ''
RPM_STATE = RPM_SCANNING

def rpm_set_state(new_state, verbose):
    global RPM_STATE
    old = RPM_STATE_TO_NAMES[RPM_STATE]
    new = RPM_STATE_TO_NAMES[new_state]
    if verbose:
        print(f'state changing from {old} to {new}')
    RPM_STATE = new_state


RPM_FILES_SECTION_PATTERN = r'(?P<files>%files)(\s+(?P<section>.*))?'
RPM_FILES_SECTION_RE = re.compile(RPM_FILES_SECTION_PATTERN, re.IGNORECASE)

def rpm_match_files_section(line, verbose):
    global RPM_SECTION
    m = RPM_FILES_SECTION_RE.match(line)
    if m and m.group('files') == '%files':
        if verbose:
            print('scanned %files', end='')
        sect = m.group('section')
        if sect:
            if verbose:
                print(f' "{RPM_SECTION}" -> "{sect}"', end='')
            RPM_SECTION = sect
        if verbose:
            print()
        return True

RPM_POST_SECTION_PATTERN = r'(?P<post>%post)(\s+)?'
RPM_POST_SECTION_RE = re.compile(RPM_POST_SECTION_PATTERN, re.IGNORECASE)

def rpm_match_post_section(line, verbose):
    m = RPM_POST_SECTION_RE.match(line)
    if m and m.group('post') == '%post':
        if verbose:
            print('scanned %post')
        return True

RPM_PREUN_SECTION_PATTERN = r'(?P<preun>%preun)(\s+)?'
RPM_PREUN_SECTION_RE = re.compile(RPM_PREUN_SECTION_PATTERN, re.IGNORECASE)

def rpm_match_preun_section(line, verbose):
    m = RPM_PREUN_SECTION_RE.match(line)
    if m and m.group('preun') == '%preun':
        if verbose:
            print('scanned %preun')
        return True

RPM_CHANGELOG_SECTION_PATTERN = r'(?P<changelog>%changelog)(\s+)?'
RPM_CHANGELOG_SECTION_RE = re.compile(RPM_CHANGELOG_SECTION_PATTERN, re.IGNORECASE)

def rpm_match_changelog_section(line, verbose):
    m = RPM_CHANGELOG_SECTION_RE.match(line)
    if m and m.group('changelog') == '%changelog':
        if verbose:
            print('scanned %changelog')
        return True


RPM_STATE_TABLE = [
    {"current": RPM_SCANNING,
     "next": RPM_COLLECTING,
     "checks": [ rpm_match_files_section ]},

    {"current": RPM_COLLECTING,
     "next": RPM_COLLECTING,
     "checks": [ rpm_match_files_section ]},

    {"current": RPM_COLLECTING,
     "next": RPM_SCANNING,
     "checks": [ rpm_match_post_section,
                 rpm_match_preun_section,
                 rpm_match_changelog_section]},

    {"current": RPM_SCANNING,
     "next": RPM_SCANNING,
     "checks": [ rpm_match_post_section,
                 rpm_match_preun_section,
                 rpm_match_changelog_section]},

]


RPM_VARIABLE_PATTERN = r'(?P<var>%{\w+})'
RPM_VARIABLE_RE = re.compile(RPM_VARIABLE_PATTERN, re.IGNORECASE)

RPM_ENVIRON_PATTERN = r'%{getenv:(?P<env>\w+)}'
RPM_ENVIRON_RE = re.compile(RPM_ENVIRON_PATTERN, re.IGNORECASE)

RPM_VARS = {
    '%{_sysconfdir}': '/etc',
    '%{_prefix}': '/usr',
    '%{_exec_prefix}': '%{_prefix}',
    '%{_includedir}': '%{_prefix}/include',
    '%{_bindir}': '%{_exec_prefix}/bin',
    '%{_libdir}': '%{_exec_prefix}/%{_lib}',
    '%{_libexecdir}': '%{_exec_prefix}/libexec',
    '%{_sbindir}': '%{_exec_prefix}/sbin',
    '%{_datadir}': '%{_datarootdir}',
    '%{_infodir}': '%{_datarootdir}/info',
    '%{_mandir}': '%{_datarootdir}/man',
    '%{_docdir}': '%{_datadir}/doc',
    '%{_rundir}': '/run',
    '%{_localstatedir}': '/var',
    '%{_sharedstatedir}': '/var/lib',
    '%{_lib}': 'lib64', # lib on 32bit platforms
    '%{_datarootdir}': '%{_prefix}/share',
    '%{_var}': '/var',
    '%{_tmppath}': '%{_var}/tmp',
    '%{_usr}': '/usr',
    '%{_usrsrc}': '%{_usr}/src',
    '%{_initddir}': '%{_sysconfdir}/rc.d/init.d',
    '%{_initrddir}': '%{_initddir}',
    '%{buildroot}': '%{_buildrootdir}/%{name}-%{version}-%{release}.%{_arch}', # same as $BUILDROOT
    '%{_topdir}': '%{getenv:HOME}/rpmbuild',
    '%{_builddir}': '%{_topdir}/BUILD',
    '%{_rpmdir}': '%{_topdir}/RPMS',
    '%{_sourcedir}': '%{_topdir}/SOURCES',
    '%{_specdir}': '%{_topdir}/SPECS',
    '%{_srcrpmdir}': '%{_topdir}/SRPMS',
    '%{_buildrootdir}': '%{_topdir}/BUILDROOT',
    '%{_unitdir}': '/lib/systemd/system'
}

def rpm_expand_variables(expr):
    while RPM_VARIABLE_RE.search(expr):
        for m in RPM_VARIABLE_RE.finditer(expr):
            v = m.group('var')
            r = RPM_VARS.get(v)
            assert r
            expr = expr.replace(v, r)
    while RPM_ENVIRON_RE.search(expr):
        for m in RPM_ENVIRON_RE.finditer(expr):
            e = m.group('env')
            v = f'%{{getenv:{e}}}'
            r = os.environ[e]
            expr = expr.replace(v, r)
    return expr


ALTERNATE_PATHS = {
    '/etc/opae/opae.cfg': [Path('~/.local/opae.cfg').expanduser().as_posix(),
                           Path('~/.local/opae/opae.cfg').expanduser().as_posix(),
                           Path('~/.config/opae/opae.cfg').expanduser().as_posix(),
                           '/usr/local/etc/opae/opae.cfg'],
    '/usr/': ['/usr/local/',
              Path('~/.local').expanduser().as_posix() + '/']
}

def alternates_for(path, platform):
    l = []
    for k in ALTERNATE_PATHS:
        if path.find(k) >= 0:
            l.extend([path.replace(k, a) for a in ALTERNATE_PATHS[k]])
    if platform == 'redhat':
        l.extend([x.replace('/lib64/', '/lib/')
                  for x in [path] + l
                  if x.find('/lib64/') >= 0])
    return l


class RemoveTree():
    def __init__(self, pattern):
        self.pattern = pattern
        if pattern[-1] == '*':
            self.directory = Path(pattern[:-1])
        else:
            self.directory = Path(pattern)

    def alternates(self, platform):
        alt = alternates_for(self.pattern, platform)
        return [RemoveTree(a) for a in alt]

    @staticmethod
    def remove_file(file, dry_run):
        if dry_run:
            print(f'file: {file}')
        else:
            print(f'rm -f {file}')
            file.unlink()

    @staticmethod
    def remove_directory(directory, dry_run):
        if dry_run:
            print(f'dir : {directory}')
        else:
            print(f'rmdir {directory}')
            directory.rmdir()

    def remove_tree(self, path, dry_run):
        subdirs = [x for x in path.iterdir() if x.is_dir()]
        for s in subdirs:
            self.remove_tree(s, dry_run)
            self.remove_directory(s, dry_run)

        files = [x for x in path.iterdir() if x.is_file() or x.is_symlink()]
        for f in files:
            self.remove_file(f, dry_run)

    def remove(self, **kwargs):
        dry_run = kwargs.get('dry_run', True)
        missing = kwargs.get('missing', False)
        try:
            d = self.directory.resolve(strict=True)
        except FileNotFoundError:
            if missing:
                print(f'missing {self.directory}')
            return
        except RuntimeError:
            print(f'loop {self.directory}')
            return
        if d.is_dir():
            self.remove_tree(d, dry_run)
            if dry_run:
                print(f'dir : {d}')
            else:
                print(f'rmdir {d}')
                d.rmdir()

    def __str__(self):
        return f'<tree: {self.pattern}>'

    def __repr__(self):
        return str(self)


class RemoveFile():
    def __init__(self, file):
        self.pattern = file
        if file[-1] == '*':
            self.file = Path(file[:-1])
        else:
            self.file = Path(file)

    def alternates(self, platform):
        alt = alternates_for(self.pattern, platform)
        return [RemoveFile(a) for a in alt]

    def remove(self, **kwargs):
        dry_run = kwargs.get('dry_run', True)
        missing = kwargs.get('missing', False)
        try:
            f = self.file.resolve(strict=True)
        except FileNotFoundError:
            if missing:
                print(f'missing {self.file}')
            return
        except RuntimeError:
            print(f'loop {self.file}')
            return
        if f.is_file() or f.is_symlink():
            if dry_run:
                print(f'file: {f}')
            else:
                print(f'rm -f {f}')
                f.unlink()

    def __str__(self):
        return f'<file: {self.pattern}>'

    def __repr__(self):
        return str(self)


class RemoveFileGlob():
    def __init__(self, pattern):
        self.pattern = pattern

    def alternates(self, platform):
        alt = alternates_for(self.pattern, platform)
        return [RemoveFileGlob(a) for a in alt]

    def remove(self, **kwargs):
        dry_run = kwargs.get('dry_run', True)
        missing = kwargs.get('missing', False)

        pure = PurePath(self.pattern)
        directory = Path(pure.parent.as_posix())
        glob_expr = str(pure.name)

        try:
            d = directory.resolve(strict=True)
        except FileNotFoundError:
            if missing:
                print(f'missing {directory}')
            return
        except RuntimeError:
            print(f'loop {directory}')
            return

        for file in d.glob(glob_expr):
            try:
                f = file.resolve(strict=True)
            except FileNotFoundError:
                if missing:
                    print(f'missing {file}')
                continue
            except RuntimeError:
                print(f'loop {file}')
                continue

            if f.is_file() or f.is_symlink():
                if dry_run:
                    print(f'file: {f}')
                else:
                    print(f'rm -f {f}')
                    f.unlink()

    def __str__(self):
        return f'<file glob: {self.pattern}>'

    def __repr__(self):
        return str(self)


RPM_DIR_PATTERN = r'(?P<dir>%dir)(\s+(?P<path>.*))'
RPM_DIR_RE = re.compile(RPM_DIR_PATTERN, re.IGNORECASE)

RPM_DOC_PATTERN = r'(?P<doc>%doc)(\s+(?P<path>.*))'
RPM_DOC_RE = re.compile(RPM_DOC_PATTERN, re.IGNORECASE)

RPM_LICENSE_PATTERN = r'(?P<license>%license)(\s+(?P<path>.*))'
RPM_LICENSE_RE = re.compile(RPM_LICENSE_PATTERN, re.IGNORECASE)

RPM_LIBDIR_PATTERN = r'(?P<libdir>%{_libdir})'
RPM_LIBDIR_RE = re.compile(RPM_LIBDIR_PATTERN, re.IGNORECASE)

RPM_BINDIR_PATTERN = r'(?P<bindir>%{_bindir})'
RPM_BINDIR_RE = re.compile(RPM_BINDIR_PATTERN, re.IGNORECASE)

RPM_PYTHON3_SITELIB_PATTERN = r'(?P<sitelib>%{python3_sitelib})'
RPM_PYTHON3_SITELIB_RE = re.compile(RPM_PYTHON3_SITELIB_PATTERN, re.IGNORECASE)

RPM_PYTHON3_SITEARCH_PATTERN = r'(?P<sitearch>%{python3_sitearch})'
RPM_PYTHON3_SITEARCH_RE = re.compile(RPM_PYTHON3_SITEARCH_PATTERN, re.IGNORECASE)

RPM_CONFIG_PATTERN = r'(?P<config>%config)(?P<noreplace>\(noreplace\))?(\s+(?P<path>.*))'
RPM_CONFIG_RE = re.compile(RPM_CONFIG_PATTERN, re.IGNORECASE)

RPM_DATA_PATTERN = r'(?P<data>%{_datadir})'
RPM_DATA_RE = re.compile(RPM_DATA_PATTERN, re.IGNORECASE)

RPM_UNITDIR_PATTERN = r'(?P<unit>%{_unitdir})'
RPM_UNITDIR_RE = re.compile(RPM_UNITDIR_PATTERN, re.IGNORECASE)

RPM_INCLUDEDIR_PATTERN = r'(?P<incdir>%{_includedir})'
RPM_INCLUDEDIR_RE = re.compile(RPM_INCLUDEDIR_PATTERN, re.IGNORECASE)

RPM_USR_SRC_PATTERN = r'(?P<usrsrc>%{_usr}/src)'
RPM_USR_SRC_RE = re.compile(RPM_USR_SRC_PATTERN, re.IGNORECASE)

RPM_PREFIX_LIB_PATTERN = r'(?P<prefixlib>%{_prefix}/lib)'
RPM_PREFIX_LIB_RE = re.compile(RPM_PREFIX_LIB_PATTERN, re.IGNORECASE)

RPM_USR_SHARE_PATTERN = r'(?P<usrshare>%{_usr}/share)'
RPM_USR_SHARE_RE = re.compile(RPM_USR_SHARE_PATTERN, re.IGNORECASE)

DIRS_TO_REMOVE = []
DOCS_TO_REMOVE = []
LICENSES_TO_REMOVE = []
LIBRARIES_TO_REMOVE = []
BINARIES_TO_REMOVE = []
PYTHON_TO_REMOVE = []
CONFIGS_TO_REMOVE = []
DATA_TO_REMOVE = []
C_CXX_TO_REMOVE = []


def infer_type(expanded):
    file_exts = ['.c',
                 '.h',
                 '.py',
                 '.pyc',
                 '.json',
                 '.cmake',
                 '.md',
                 '.txt',
                 '.so',
                 '.qsf',
                 '.sdc',
                 '.tcl',
                 '.service',
                 '.template',
                 '.conf',
                 '.sv',
                 '.vh',
                 '.sh',
                 '.v']

    apps = ['fpgad',
            'fpgaconf',
            'fpgainfo',
            'fpgasupdate',
            'rsu',
            'pci_device',
            'bitstreaminfo',
            'fpgaflash',
            'fpgaotsu',
            'fpgaport',
            'super-rsu',
            'mmlink',
            'userclk',
            'qsfpinfo',
            'hello_fpga',
            'object_api',
            'hello_events',
            'hello_cxxcore',
            'afu_json_mgr',
            'packager',
            'fpgametrics',
            'n5010-test',
            'n5010-ctl',
            'PACSign',
            'opaevfio',
            'opaevfiotest',
            'regmap-debugfs',
            'fpgareg',
            'afu_platform_config',
            'afu_platform_info',
            'afu_synth_setup',
            'hssiloopback',
            'hssimac',
            'hssistats',
            'opaeuiotest',
            'rtl_src_config',
            'nlb0',
            'nlb3',
            'nlb7',
            'vabtool',
            'bist_app',
            'dummy_afu',
            'fpgabist',
            'fecmode',
            'fpgamac',
            'fvlbypass',
            'mactest',
            'fpgadiag',
            'fpgalpbk',
            'fpgastats',
            'fpga_dma_N3000_test',
            'fpga_dma_test',
            'host_exerciser',
            'cxl_mem_tg',
            'bist',
            'hps',
            'hssi',
            'opae.io',
            'mem_tg']

    pure = PurePath(expanded)
    base = Path(pure.parent.as_posix())
    file = str(pure.name)
    flen = len(file)

    if flen >= 2 and file[-2:] == '.*':
        return RemoveFileGlob
    if flen >= 3 and file[-3:] == '*.h':
        return RemoveFileGlob

    for e in file_exts:
        elen = len(e)
        if flen >= elen and file[-elen:] == e:
            return RemoveFile

    if file in apps:
        return RemoveFile

    return RemoveTree


def rpm_process(line, deep):
    expanded = rpm_expand_variables(line)
    plat = 'redhat'

    m = RPM_DIR_RE.match(expanded)
    if m:
        r = RemoveTree(m.group('path'))
        DIRS_TO_REMOVE.append(r)
        if deep:
            DIRS_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_DOC_RE.match(expanded)
    if m:
        cls = infer_type(expanded)
        r = cls(m.group('path'))
        DOCS_TO_REMOVE.append(r)
        if deep:
            DOCS_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_LICENSE_RE.match(expanded)
    if m:
        r = RemoveFile(m.group('path'))
        LICENSES_TO_REMOVE.append(r)
        if deep:
            LICENSES_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_LIBDIR_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        LIBRARIES_TO_REMOVE.append(r)
        if deep:
            LIBRARIES_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_BINDIR_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        BINARIES_TO_REMOVE.append(r)
        if deep:
            BINARIES_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_PYTHON3_SITELIB_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        PYTHON_TO_REMOVE.append(r)
        if deep:
            PYTHON_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_PYTHON3_SITEARCH_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        PYTHON_TO_REMOVE.append(r)
        if deep:
            PYTHON_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_CONFIG_RE.match(expanded)
    if m:
        r = RemoveFile(m.group('path'))
        CONFIGS_TO_REMOVE.append(r)
        if deep:
            CONFIGS_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_DATA_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        DATA_TO_REMOVE.append(r)
        if deep:
            DATA_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_UNITDIR_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        CONFIGS_TO_REMOVE.append(r)
        if deep:
            CONFIGS_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_INCLUDEDIR_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        C_CXX_TO_REMOVE.append(r)
        if deep:
            C_CXX_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_USR_SRC_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        C_CXX_TO_REMOVE.append(r)
        if deep:
            C_CXX_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_PREFIX_LIB_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        C_CXX_TO_REMOVE.append(r)
        if deep:
            C_CXX_TO_REMOVE.extend(r.alternates(plat))
        return

    m = RPM_USR_SHARE_RE.match(line)
    if m:
        cls = infer_type(expanded)
        r = cls(expanded)
        DATA_TO_REMOVE.append(r)
        if deep:
            DATA_TO_REMOVE.extend(r.alternates(plat))
        return

    print(f'unmatched {line}')


def rpm_process_spec(args):
    global RPM_VARS
    RPM_VARS['%{python3_sitelib}'] = subprocess.check_output([
        'rpm', '-E', "%{python3_sitelib}"]).decode().rstrip()
    RPM_VARS['%{python3_sitearch}'] = subprocess.check_output([
        'rpm', '-E', "%{python3_sitearch}"]).decode().rstrip()

    RPM_VARS['%{name}'] = args.project_name
    RPM_VARS['%{version}'] = args.project_version
    RPM_VARS['%{release}'] = args.project_release
    RPM_VARS['%{_arch}'] = args.build_arch

    with open(args.specfile, 'r') as inf:
        for line in inf:
            line = line.rstrip()

            state_changed = False
            for state in RPM_STATE_TABLE:
                if RPM_STATE == state['current']:
                    for check in state['checks']:
                        if check(line, args.state):
                            state_changed = True
                            break
                if state_changed:
                    rpm_set_state(state['next'], args.state)
                    break

            # The lines that trigger state changes don't
            # represent files or directories installed by
            # the RPM.
            if not state_changed:
                if line and RPM_STATE == RPM_COLLECTING:
                    rpm_process(line, args.deep)


def deb_expand_variables(line, args):
    if line.find('@VERSION@') >= 0:
        line = line.replace('@VERSION@', args.project_version)
    return line


DEB_FILE_THEN_PATH_PATTERN = r'(?P<file>(\w|\.)+)\s+(?P<path>.*)'
DEB_FILE_THEN_PATH_RE = re.compile(DEB_FILE_THEN_PATH_PATTERN, re.IGNORECASE)

DEB_LIBRARY_FILE_GLOB_PATTERN = r'^(?P<glob>(\w|[/])+lib(\w|[-+])+\.so\.\*)'
DEB_LIBRARY_FILE_GLOB_RE = re.compile(DEB_LIBRARY_FILE_GLOB_PATTERN, re.IGNORECASE)

DEB_LIBRARY_FILE_PATTERN = r'(?P<file>(\w|[/])+lib(\w|[-+])+\.(so|a))'
DEB_LIBRARY_FILE_RE = re.compile(DEB_LIBRARY_FILE_PATTERN, re.IGNORECASE)

DEB_BINARY_FILE_PATTERN = r'(?P<bin>(\w|[/])+bin/(\w|[-+])+)'
DEB_BINARY_FILE_RE = re.compile(DEB_BINARY_FILE_PATTERN, re.IGNORECASE)

DEB_PYTHON_DIR_GLOB_PATTERN = r'(?P<path>(\w|[/])+python3/dist-packages/.*[*])'
DEB_PYTHON_DIR_GLOB_RE = re.compile(DEB_PYTHON_DIR_GLOB_PATTERN, re.IGNORECASE)

DEB_CONFIG_FILES_PATTERN = r'(?P<path>(\w|[/])+(fpgad\.conf|fpgad\.service|opae\.cfg))'
DEB_CONFIG_FILES_RE = re.compile(DEB_CONFIG_FILES_PATTERN, re.IGNORECASE)

DEB_HEADER_GLOB_PATTERN = r'(?P<glob>.*[*]\.h)'
DEB_HEADER_GLOB_RE = re.compile(DEB_HEADER_GLOB_PATTERN, re.IGNORECASE)

DEB_C_FILE_PATTERN = r'(?P<file>.*\.(c|h))'
DEB_C_FILE_RE = re.compile(DEB_C_FILE_PATTERN, re.IGNORECASE)

DEB_DIR_GLOB_PATTERN= r'(?P<glob>(\w|[/])+[/][*])'
DEB_DIR_GLOB_RE = re.compile(DEB_DIR_GLOB_PATTERN, re.IGNORECASE)

def deb_process_install_file(path, args):
    deep = args.deep
    plat = 'debian'

    with open(path, 'r') as inf:
        for line in inf:
            line = deb_expand_variables(line.rstrip(), args)

            m = DEB_FILE_THEN_PATH_RE.match(line)
            if m:
                file = Path(m.group('file'))
                p = Path(m.group('path'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFile(p.as_posix() + os.sep + file.as_posix())
                LICENSES_TO_REMOVE.append(r)
                if deep:
                    LICENSES_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_LIBRARY_FILE_GLOB_RE.match(line)
            if m:
                p = Path(m.group('glob'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFileGlob(p.as_posix())
                LIBRARIES_TO_REMOVE.append(r)
                if deep:
                    LIBRARIES_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_LIBRARY_FILE_RE.match(line)
            if m:
                p = Path(m.group('file'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFile(p.as_posix())
                LIBRARIES_TO_REMOVE.append(r)
                if deep:
                    LIBRARIES_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_BINARY_FILE_RE.match(line)
            if m:
                p = Path(m.group('bin'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFile(p.as_posix())
                BINARIES_TO_REMOVE.append(r)
                if deep:
                    BINARIES_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_PYTHON_DIR_GLOB_RE.match(line)
            if m:
                p = Path(m.group('path'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveTree(p.as_posix())
                PYTHON_TO_REMOVE.append(r)
                if deep:
                    PYTHON_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_CONFIG_FILES_RE.match(line)
            if m:
                p = Path(m.group('path'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFile(p.as_posix())
                CONFIGS_TO_REMOVE.append(r)
                if deep:
                    CONFIGS_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_HEADER_GLOB_RE.match(line)
            if m:
                p = Path(m.group('glob'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFileGlob(p.as_posix())
                C_CXX_TO_REMOVE.append(r)
                if deep:
                    C_CXX_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_C_FILE_RE.match(line)
            if m:
                p = Path(m.group('file'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveFile(p.as_posix())
                C_CXX_TO_REMOVE.append(r)
                if deep:
                    C_CXX_TO_REMOVE.extend(r.alternates(plat))
                continue

            m = DEB_DIR_GLOB_RE.match(line)
            if m:
                p = Path(m.group('glob'))

                if str(p)[0] != os.sep:
                    p = Path(os.sep, p)

                r = RemoveTree(p.as_posix())
                DIRS_TO_REMOVE.append(r)
                if deep:
                    DIRS_TO_REMOVE.extend(r.alternates(plat))
                continue

            print(f'unmatched {line}')


def deb_process_dirs_file(path, args):
    deep = args.deep
    plat = 'debian'

    with open(path, 'r') as inf:
        for line in inf:
            line = deb_expand_variables(line.rstrip(), args)

            if line[0] != os.sep:
                line = os.sep + line

            r = RemoveTree(line)
            DIRS_TO_REMOVE.append(r)
            if deep:
                DIRS_TO_REMOVE.extend(r.alternates(plat))


def deb_process_files(args):
    specdir = Path(args.specdir)

    for inst in specdir.glob('*.install'):
        deb_process_install_file(inst, args)

    for d in specdir.glob('*.dirs'):
        deb_process_dirs_file(d, args)


def rpm_spec_file(filename):
    try:
        script = Path(__file__).resolve(strict=True)
    except (FileNotFoundError, RuntimeError):
        return None

    # The RPM specs live in ../ relative to this script.
    parent = PurePath(script).parent.parent
    spec = Path(parent.joinpath(filename))
    if spec.is_file():
        return spec.as_posix()


def deb_project_path(path):
    try:
        script = Path(__file__).resolve(strict=True)
    except (FileNotFoundError, RuntimeError):
        return None

    # The DEB project lives in ../ relative to this script.
    parent = PurePath(script).parent.parent
    proj = Path(parent.joinpath(path))
    if proj.is_dir():
        return proj.as_posix()


def parse_args(platform):
    parser = argparse.ArgumentParser()

    if platform == 'redhat':
        parser.add_argument('specfile', nargs='?',
                            type=rpm_spec_file,
                            default='opae.spec.fedora',
                            help='Path to the RPM spec to guide '
                                 'the search [%(default)s]')
        parser.add_argument('-a', '--build-arch', nargs='+', default='x86_64',
                            help='specify the architecture used when creating '
                                 'the RPM [%(default)s]')
        parser.add_argument('-n', '--project-name', nargs='+', default='opae',
                            help='specify the project name used in the '
                                 'RPM spec [%(default)s]')
        parser.add_argument('-V', '--project-version', nargs='+', default='2.2.0',
                            help='specify the project version used in the '
                                 'RPM spec [%(default)s]')
        parser.add_argument('-r', '--project-release', nargs='+', default='1',
                            help='specify the project release used in the '
                                 'RPM spec [%(default)s]')
        parser.add_argument('-s', '--state', default=False, action='store_true',
                            help='show state change information as the '
                                 'RPM spec is parsed')
    elif platform == 'debian':
        parser.add_argument('specdir', nargs='?',
                            type=deb_project_path,
                            default='packaging/opae/deb',
                            help='Path to the DEB spec files to guide '
                                 'the search [%(default)s]')
        parser.add_argument('-V', '--project-version', nargs='+', default='2.2.0',
                            help='specify the project version used in the '
                                 'DEB project files [%(default)s]')

    parser.add_argument('-d', '--deep', default=False, action='store_true',
                        help='search in alternate directories, eg /usr/local')
    parser.add_argument('-m', '--missing', default=False, action='store_true',
                        help='print a message for files and directories '
                             'that were not found')
    parser.add_argument('-D', '--delete', default=False, action='store_true',
                        help='remove all detected files and directories')
    parser.add_argument('-v', '--version', action='version',
                        version=f"%(prog)s 0.0.0",
                        help='display version information and exit')
    return parser, parser.parse_args()


def main():
    platforms = {'redhat': '/etc/redhat-release',
                 'debian': '/etc/lsb-release'}

    platform = None
    for key in platforms:
        try:
            Path(platforms[key]).resolve(strict=True)
            platform = key
        except (FileNotFoundError, RuntimeError):
            pass

    if not platform:
        print('Platform detection failed. Exiting')
        sys.exit(1)

    parser, args = parse_args(platform)

    if platform == 'redhat':
        rpm_process_spec(args)
    elif platform == 'debian':
        deb_process_files(args)

    dry_run = not args.delete
    missing = args.missing

    for d in DOCS_TO_REMOVE:
        d.remove(dry_run=dry_run, missing=missing)

    for l in LICENSES_TO_REMOVE:
        l.remove(dry_run=dry_run, missing=missing)

    for l in LIBRARIES_TO_REMOVE:
        l.remove(dry_run=dry_run, missing=missing)

    for b in BINARIES_TO_REMOVE:
        b.remove(dry_run=dry_run, missing=missing)

    for p in PYTHON_TO_REMOVE:
        p.remove(dry_run=dry_run, missing=missing)

    for c in CONFIGS_TO_REMOVE:
        c.remove(dry_run=dry_run, missing=missing)

    for d in DATA_TO_REMOVE:
        d.remove(dry_run=dry_run, missing=missing)

    for c in C_CXX_TO_REMOVE:
        c.remove(dry_run=dry_run, missing=missing)

    for d in DIRS_TO_REMOVE:
        d.remove(dry_run=dry_run, missing=missing)


if __name__ == '__main__':
    main()
