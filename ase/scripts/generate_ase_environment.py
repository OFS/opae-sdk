#!/usr/bin/env python
# Copyright(c) 2013-2017, Intel Corporation
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

#
# generate_ase_environment.py : Generate AFU paths, include directories as a
# Makefile snippet for ASE builds
#
# Author: Rahul R Sharma <rahul.r.sharma@intel.com>
#
# After running script, you will see:
# * A list of VHDL files
# * A list of {System}Verilog files
# * ase_common.mk: Prepares variables pointing to the file lists and
#   gueses include directories. The ASE files' absolute paths are also
#   calculated
# * A synopsys_sim.setup file, applicable to VCS builds, QUESTA will
#   ignore this
#
# Mar 2014                RRS                 Original version
# Oct 2014                RRS                 Python 3.3 compatibility
#                                             Version check added
# May 2017                RRS                 Platform type support added
#
#

# Future-proofing against Python 3 syntax changes in 'print'
from __future__ import print_function
import argparse
import ase_functions
import os
import sys
import subprocess
from collections import defaultdict
import fnmatch
import json
from sets import Set
import shutil

reload(sys)
sys.setdefaultencoding('utf8')

# Supported file extensions
# USERs may modify this if needed
VLOG_EXTENSIONS = [".svh", ".sv", ".vs", ".v"]
VHD_EXTENSIONS = [".vhd", ".vhdl"]

VHDL_FILE_LIST = os.getcwd() + "/vhdl_files.list"
VLOG_FILE_LIST = os.getcwd() + "/vlog_files.list"

# Forbidden characters
SPECIAL_CHARS = '\\[]~!@#$%^&*(){}:;+$\''


# DO NOT MODIFY BELOW THIS COMMENT BLOCK     #
# Global variables
arg_list = []
tolowarg_list = []
valid_dirlist = []


def errorExit(msg):
    ase_functions.begin_red_fontcolor()
    sys.stderr.write("Error: " + msg + "\n")
    ase_functions.end_red_fontcolor()

    # Try to remove ase_sources.mk to make it clear something went wrong
    try:
        os.remove('ase_sources.mk')
    except Exception:
        None

    sys.exit(1)


def remove_dups(filepath, exclude=None):
    import hashlib

    def include(f):
        if exclude is None or not fnmatch.fnmatch(f, exclude):
            return True
        else:
            return False
    files = []
    hashes = dict()
    with open(filepath, 'r') as fd:
        files = filter(lambda l: l.strip() != '', fd.readlines())
    files = [f.strip() for f in files]
    for f in files:
        with open(f, 'r') as fd:
            m = hashlib.md5()
            m.update(fd.read())
        h = m.digest()
        if h not in hashes and include(f):
            hashes[h] = f
    text = '\n'.join(sorted(hashes.values(), key=os.path.basename))
    with open(filepath, 'w') as fd:
        fd.write(text)
        fd.write("\n")
    return text


# Find files in a directory
def search_file(pattern, cur=os.curdir):
    filelist = []
    for dir, subdirs, files in os.walk(os.path.abspath(cur)):
        for file in fnmatch.filter(files, pattern):
            filelist.append(os.path.join(os.path.abspath(dir), file))
    return filelist


# Find dirs in a directory
def search_dir(pattern, cur=os.curdir):
    dirlist = []
    for dir, subdirs, files in os.walk(os.path.abspath(cur)):
        for d in subdirs:
            dirlist.append(os.path.join(dir, d))
    return dirlist


# Run a command
def commands_list(cmd, cwd=None, stdout=None):
    try:
        subprocess.check_call(cmd, cwd=cwd, stdout=stdout)
    except OSError as e:
        if e.errno == os.errno.ENOENT:
            msg = cmd[0] + " not found on PATH!"
            errorExit(msg)
        else:
            raise
    except subprocess.CalledProcessError as e:
        errorExit('"' + ' '.join(e.cmd) + '" failed')
    except AttributeError:
        sys.stderr.write('Error: Python 2.7 or greater required.\n')
        raise

    return None


# Run a command and get the output
def commands_list_getoutput(cmd):
    try:
        byte_out = subprocess.check_output(cmd)
        str_out = byte_out.decode()
    except OSError as e:
        if e.errno == os.errno.ENOENT:
            msg = cmd[0] + " not found on PATH!\n"
            msg += "The installed OPAE SDK bin directory must be on " + \
                   "the PATH environment variable."
            errorExit(msg)
        else:
            raise
    except subprocess.CalledProcessError as e:
        ase_functions.begin_red_fontcolor()
        sys.stderr.write(e.output)
        ase_functions.end_red_fontcolor()
        raise

    return str_out


# Has duplicates #
def has_duplicates(word_dict):
    dups = filter(lambda (k, v): len(v) > 1, word_dict.items())
    if dups:
        print("Duplicates found -")
        for k, v in dups:
            print(k)
            print('\n'.join(['\t{}'.format(l) for l in v]))
        return True
    return False


#
# Case #1: Construct build from a list of files
#
def config_sources(fd, filelist):
    # Get all the sources.  rtl_src_config will emit all relevant source
    # files, one per line.
    try:
        srcs = commands_list_getoutput(
            "rtl_src_config --sim --abs".split(" ") + [filelist])
    except Exception:
        errorExit("failed to read sources from {0}".format(filelist))

    vlog_srcs = []
    vhdl_srcs = []
    json_srcs = []

    # Separate boolean tracking for whether vlog/vhdl are found since
    # simulator commands (e.g. +define+) wind up in both.
    vlog_found = False
    vhdl_found = False

    srcs = srcs.split('\n')
    for s in srcs:
        if (len(s) == 0):
            None
        elif (s[0] == '+'):
            # + simulator commands go only in Verilog.  VHDL simulation
            # doesn't support these.
            vlog_srcs.append(s)
        elif (s[0] == '-'):
            # For now assume - is an include directive and used only for
            # Verilog. Escape all but the first space, which likely
            # follows a simulator command.
            spl = s.split(' ')
            if (len(spl) > 1):
                s = spl[0] + ' ' + '\\ '.join(spl[1:])
            vlog_srcs.append(s)
            vlog_found = True
        else:
            # Convert extensions to lower case for comparison
            sl = s.lower()
            # Escape spaces in pathnames
            s = s.replace(' ', '\\ ')

            # Verilog or SystemVerilog?
            for ext in VLOG_EXTENSIONS:
                if (sl.endswith(ext)):
                    vlog_srcs.append(s)
                    vlog_found = True
                    break

            # VHDL?
            for ext in VHD_EXTENSIONS:
                if (sl.endswith(ext)):
                    vhdl_srcs.append(s)
                    vhdl_found = True
                    break

            if (sl.endswith('.json')):
                json_srcs.append(s)

    qsys_sim_files = config_qsys_sources(filelist, vlog_srcs)

    # List Verilog & SystemVerilog sources in a file
    if (vlog_found or qsys_sim_files):
        fd.write("DUT_VLOG_SRC_LIST = " + VLOG_FILE_LIST + " \n\n")
        with open(VLOG_FILE_LIST, "w") as f:
            for s in vlog_srcs:
                f.write(s + "\n")
            if (qsys_sim_files):
                f.write("-F " + qsys_sim_files + "\n")

    # List VHDL sources in a file
    if (vhdl_found):
        fd.write("DUT_VHD_SRC_LIST = " + VHDL_FILE_LIST + " \n\n")
        with open(VHDL_FILE_LIST, "w") as f:
            for s in vhdl_srcs:
                f.write(s + "\n")

    # Is there a JSON file describing the AFU?
    json_file = None
    if (len(json_srcs)):
        # Yes, JSON specified in file list
        json_file = json_srcs[0]
    else:
        # Is there a JSON file in the same directory as the file list?
        dir = os.path.dirname(filelist)
        files = fnmatch.filter(os.listdir(dir), "*.json")
        for file in files:
            json_file = file

        if (json_file is not None):
            # Use the discovered JSON file, but complain that it should
            # have been named explicitly.
            ase_functions.begin_green_fontcolor()
            json_basename = os.path.basename(json_file)
            print(
                " *** JSON file {0} should be included in {1} ***".format(
                    json_basename,
                    os.path.abspath(filelist)))
            print(
                "     The afu-image:afu-top-interface key in " +
                json_basename + " will be used")
            print("     to specify the AFU's top level interface.")
            ase_functions.end_green_fontcolor()

    return json_file


def remove_prefix(text, prefix):
    if text.startswith(prefix):
        return text[len(prefix):]
    return text


#
# Qsys has a compilation step.  When using case #1 above, detect Qsys
# sources, compile them, and include the generated Verilog in the
# simulation.
#
def config_qsys_sources(filelist, vlog_srcs):
    # Get all the sources.  rtl_src_config will emit all relevant source
    # files, one per line.
    try:
        srcs = commands_list_getoutput(
            "rtl_src_config --qsys --abs".split(" ") + [filelist])
    except Exception:
        errorExit("failed to read sources from {0}".format(filelist))

    # Grab _hw.tcl sources
    # _hw.tcl sources are used to describe and package
    # components used in a Qsys system. They must be available
    # in the simulation dir relative to the path specified in
    # components.ipx with Qsys system
    try:
        tsrcs = commands_list_getoutput(
            "rtl_src_config --tcl --abs".split(" ") + [filelist])
    except Exception:
        errorExit("failed to read sources from {0}".format(filelist))

    # Collect two sets: qsys source files and the names of directories into
    # which generated Verilog will be written.  The directory names match
    # the source names.
    qsys_srcs = []
    ip_dirs = []
    srcs = srcs.split('\n')
    for s in srcs:
        if (s):
            # Record all build target directories
            ip_dirs.append(os.path.splitext(s)[0])

            # Collect all qsys files
            if (s.lower().endswith('.qsys')):
                qsys_srcs.append(s)

    tcl_srcs = []
    tsrcs = tsrcs.split('\n')

    # filter tcl files
    for s in tsrcs:
        if (s):
            if (s.lower().endswith('.tcl')):
                tcl_srcs.append(s)

    # Any Qsys files found?
    if (not qsys_srcs):
        return None

    # First step: copy the trees holding Qsys sources to a temporary tree
    # inside the simulator environment.  We do this to avoid polluting the
    # source tree with Qsys-generated files.
    copied_qsys_dirs = dict()
    copied_tcl_dirs = dict()
    tgt_idx = 0
    os.mkdir('qsys_sim')
    qsys_srcs_copy = []
    tcl_srcs_copy = []
    for q in qsys_srcs:
        src_dir = os.path.dirname(q)
        base_filelist = os.path.dirname(filelist)
        paths = [src_dir, base_filelist]
        common_prefix = os.path.commonprefix(paths)
        # Has the source been copied already? Multiple Qsys files in the same
        # directory are copied together.
        if (src_dir not in copied_qsys_dirs):
            src_dir_base = os.path.basename(src_dir)
            b = remove_prefix(src_dir, common_prefix)
            b = b.strip("/")
            tgt_dir = os.path.join('qsys_sim', b)
            copied_qsys_dirs[src_dir] = tgt_dir
            print("Preparing {0}:".format(q))
            print("  Copying {0} to {1}...".format(src_dir, tgt_dir))
            try:
                shutil.copytree(src_dir, tgt_dir)
            except Exception:
                errorExit("Failed to copy tree {0} to {1}".format(src_dir,
                                                                  tgt_dir))

        # Point to the copy
        qsys_srcs_copy.append(tgt_dir + q[len(src_dir):])

    for t in tcl_srcs:
        src_dir = os.path.dirname(t)
        base_filelist = os.path.dirname(filelist)
        paths = [src_dir, base_filelist]
        common_prefix = os.path.commonprefix(paths)
        # Has the source been copied already? Multiple Qsys files in the same
        # directory are copied together.
        if (src_dir not in copied_tcl_dirs):
            src_dir_base = os.path.basename(src_dir)
            b = remove_prefix(src_dir, common_prefix)
            b = b.strip("/")
            tgt_dir = os.path.join('qsys_sim', b)
            copied_tcl_dirs[src_dir] = tgt_dir
            print("Preparing {0}:".format(t))
            print("  Copying {0} to {1}...".format(src_dir, tgt_dir))
            try:
                shutil.copytree(src_dir, tgt_dir)
            except Exception:
                errorExit("Failed to copy tree {0} to {1}".format(src_dir,
                                                                  tgt_dir))
        tcl_srcs_copy.append(tgt_dir + q[len(src_dir):])

    # Second step: now that the trees are copied, update the paths in
    # ip_dirs to point to the copies.
    ip_dirs_copy = []
    for d in ip_dirs:
        match = None
        for src in copied_qsys_dirs:
            if (src == d[:len(src)]):
                match = src
                break
        if match:
            # Replace the prefix (source tree) with the copied prefix
            ip_dirs_copy.append(copied_qsys_dirs[match] + d[len(match):])
        else:
            # Didn't find a match.  Use the original.
            ip_dirs_copy.append(d)

    # Now we are finally ready to run qsys-generate.
    try:
        cmd = [os.path.join(os.environ['QUARTUS_HOME'], 'sopc_builder',
                            'bin', 'qsys-generate')]
    except KeyError as k:
        errorExit("Required environment variable {0} is undefined".format(k))

    # We use synthesis mode instead of simulation because the generated
    # simulation control isn't needed for ASE and because some Qsys
    # projects are set up only for synthesis.
    cmd.append('--synthesis=VERILOG')

    for q in qsys_srcs_copy:
        cmd.append(q)
        print("\nBuilding " + q)
        try:
            sys.stdout.write(commands_list_getoutput(cmd))
        except Exception:
            errorExit("Qsys failure: " + " ".join(cmd))
        cmd.pop()

    print("Done building Qsys files.\n")

    # Qsys replicates library files in the tree.  Ensure that each module is
    # imported only once.
    sim_files_found = Set()
    for s in vlog_srcs:
        sim_files_found.add(os.path.basename(s))

    # Find all generated Verilog/SystemVerilog sources
    qsys_sim_files = 'qsys_sim_files.list'
    with open(qsys_sim_files, "w") as f:
        for d in ip_dirs_copy:
            for dir, subdirs, files in os.walk(d):
                for fn in files:
                    if ((os.path.basename(dir) == 'synth') and
                            (fn.endswith('.v') or fn.endswith('.sv'))):
                        full_path = os.path.join(dir, fn)
                        # Is the module (file) name new?
                        if (fn not in sim_files_found and
                                full_path not in sim_files_found):
                            sim_files_found.add(fn)
                            # Escape spaces in pathnames
                            full_path = full_path.replace(' ', '\\ ')
                            f.write(full_path + "\n")

    return qsys_sim_files


#
# Case #2: Construct build automatically by looking for RTL sources
#
def auto_find_sources(fd):
    # Prepare list of candidate directories
    print("Valid directories supplied => ")
    valid_dirlist = filter(lambda p: os.path.exists(p), args.dirlist)
    str_dirlist = " ".join(valid_dirlist)
    if len(valid_dirlist) == 0:
        # This line should never be reached since the directory list was
        # already checked after argument parsing.
        errorExit("No source directories specifield")

    # Check if VHDL files exist, populate if any
    print("")
    print("Finding VHDL files ... ")
    str = ""
    vhdl_filepaths = ""
    for extn in VHD_EXTENSIONS:
        for dir in valid_dirlist:
            for file in search_file("*"+extn, dir):
                str = str + file + '\n'
        if len(str) != 0:
            str = str + "\n"
    if len(str.strip()) != 0:
        open(VHDL_FILE_LIST, "w").write(str)
        vhdl_filepaths = str
        print("DUT_VHD_SRC_LIST = " + VHDL_FILE_LIST)
        fd.write("DUT_VHD_SRC_LIST = " + VHDL_FILE_LIST + " \n\n")
    else:
        print("No VHDL files were found !")

    # Check if V/SV files exist, populate if any
    print("")
    print("Finding {System}Verilog files ... ")
    str = ""
    pkgfiles = []
    vlog_filepaths = ""
    cmd = ""
    for extn in VLOG_EXTENSIONS:
        for dir in valid_dirlist:
            pkgfiles = search_file("*pkg*" + extn, dir)
            for file in pkgfiles:
                str = str + file + '\n'
    for extn in VLOG_EXTENSIONS:
        for dir in valid_dirlist:
            for file in search_file("*"+extn, dir):
                if file not in pkgfiles:
                    str = str + file + '\n'
    if len(str) != 0:
        open(VLOG_FILE_LIST, "w").write(str)
        vlog_filepaths = str
        print("DUT_VLOG_SRC_LIST = " + VLOG_FILE_LIST)
        fd.write("DUT_VLOG_SRC_LIST = " + VLOG_FILE_LIST + " \n\n")
    else:
        print("No {System}Verilog files were found !")

    vlog_filepaths = remove_dups(VLOG_FILE_LIST, args.exclude)

    # Recursively find and add directory locations for VH
    print("")
    print("Finding include directories ... ")

    # use absolute path names in DUT_INCDIR to keep Questa happy
    str = ""
    for dir in valid_dirlist:
        for file in search_dir("*", dir):
            str = str + file + '\n'

    str = str.replace("\n", "+")
    if len(str) != 0:
        print("DUT_INCDIR = " + str)
        fd.write("DUT_INCDIR = " + str + "\n\n")

    # Module repetition check
    vhdl_filepaths = vhdl_filepaths.replace("\n", " ").split()
    vlog_filepaths = vlog_filepaths.replace("\n", " ").split()

    all_filepaths = vhdl_filepaths + vlog_filepaths
    module_namelist = []
    module_files = defaultdict(list)

    for filepath in all_filepaths:
        file_content = open(filepath).readlines()
        for line in file_content:
            strip_line = line.strip()
            if strip_line.startswith("//"):
                continue
            elif strip_line.startswith("module"):
                words = strip_line.split()
                modname = words[1]
                module_files[modname].append(filepath)
                module_namelist.append(modname)

    if (has_duplicates(module_files)):
        ase_functions.begin_red_fontcolor()
        print("\n")
        print("Duplicate module names were found in the RTL file lists.")
        print("Please remove them manually as RTL compilation is expected " +
              "to FAIL !")
        ase_functions.end_red_fontcolor()

    # Search for a JSON file describing the AFU
    json_file = None
    str = ""
    for dir in valid_dirlist:
        for file in search_file("*.json", dir):
            str = file
    if (len(str)):
        for js in str.split('\n'):
            try:
                with open(js, 'r') as f:
                    db = json.load(f)
                f.close()

                afu_image = db['afu-image']
                # If we get this far without an exception the JSON file looks
                # like an AFU descriptor.
                json_file = js
                break
            except ValueError:
                ase_functions.begin_red_fontcolor()
                sys.stderr.write("Error: reading JSON file {0}".format(js))
                ase_functions.end_red_fontcolor()
                raise
            except KeyError:
                # Ignore key error -- maybe the file isn't an AFU descriptor
                None

    # Print auto-find instructions
    print("")
    ase_functions.begin_green_fontcolor()
    print("NOTES TO USER => ")
    print("* This script assumes File Extensions: ")
    print("  * VHDL : .vhd")
    print("  * V/SV : .sv .vs .v")
    print('  * If you use arbitrary extensions, please edit this script to '
          'reflect them, and re-run the script')
    print("* See ase_sources.mk and check for correctness")
    print("* See if DUT_INCDIR has all the locations mentioned")
    print("  * If a directory is missing, append it separated by '+' symbol")
    ase_functions.end_green_fontcolor()
    print("")

    return json_file


# AFU / Platform interface configuration
def gen_afu_platform_ifc(json_file):
    cmd = "afu_platform_config"
    cfg = (cmd + " --sim --tgt=rtl").split(" ")

    default_ifc = "ccip_std_afu"
    if (args.platform == 'discrete'):
        default_ifc = "ccip_std_afu_avalon_mm_legacy_wires"

    if (json_file):
        cfg.append("--src=" + json_file)
        cfg.append("--default-ifc=" + default_ifc)
    else:
        cfg.append("--ifc=" + default_ifc)

    if (args.platform == 'discrete'):
        # Compatibility mode for original "discrete" platform argument.
        cfg.append("discrete_pcie3")
    else:
        cfg.append(args.platform)

    commands_list(cfg)


# Generate a Verilog header file with values from the AFU JSON file
def gen_afu_json_verilog_macros(json_file):
    cmd = ['afu_json_mgr', 'json-info']
    cmd.append('--afu-json=' + json_file)
    cmd.append('--verilog-hdr=rtl/afu_json_info.vh')

    commands_list_getoutput(cmd)


# What's the default platform?  Environment variables allow developers
# to set a default for their projects.
def get_default_platform():
    if 'OPAE_ASE_DEFAULT_PLATFORM' in os.environ:
        return os.environ['OPAE_ASE_DEFAULT_PLATFORM']

    # FPGA platform releases store the platform class in their
    # hw/lib tree.  There are two variables that may point to
    # this tree.
    if ('BBS_LIB_PATH' in os.environ):
        # Legacy variable, shared with afu_synth_setup and HW releases
        hw_lib_dir = os.environ['BBS_LIB_PATH'].rstrip('/')
    elif ('OPAE_PLATFORM_ROOT' in os.environ):
        # Currently documented variable, pointing to a platform release
        hw_lib_dir = os.path.join(os.environ['OPAE_PLATFORM_ROOT'].rstrip('/'),
                                  'hw/lib')
    else:
        hw_lib_dir = None

    # The release library stores the platform class.  Match the ASE
    # environment to the current platform.
    if hw_lib_dir is not None:
        try:
            fname = os.path.join(hw_lib_dir, 'fme-platform-class.txt')
            with open(fname, 'r') as fd:
                fpga_platform = fd.read().strip()
            return fpga_platform
        except Exception:
            ase_functions.begin_red_fontcolor()
            sys.stderr.write('Warning: expected to find FPGA platform ' +
                             'in {0}\n\n'.format(fname))
            ase_functions.end_red_fontcolor()

    return 'intg_xeon'


# The Platform Interface Manager maps the simulated platform name to
# an ASE platform name.  Load it.
def get_ase_platform():
    try:
        # This file is created as a side effect of running afu_platform_config
        with open('rtl/ase_platform_name.txt', 'r') as fd:
            ase_platform = fd.read().strip()
    except Exception:
        errorExit("ASE platform name expected in rtl/ase_platform_name.txt")

    valid_platform_names = ['intg_xeon', 'discrete']
    if (ase_platform not in valid_platform_names):
        errorExit("Unexpected ASE platform name: {0}".format(ase_platform))

    return ase_platform


print("#################################################################")
print("#                                                               #")
print("#             OPAE Intel(R) Xeon(R) + FPGA Library              #")
print("#               AFU Simulation Environment (ASE)                #")
print("#                                                               #")
print("#################################################################")
print("")
parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description="Configure an ASE environment given a set of RTL sources.",
    epilog='''\
Two modes of specifying sources are available.  When the --sources argument
is set, sources are specified explicitly.  The rtl_src_config script,
included in OPAE, parses the sources file.  rtl_src_config supports
setting preprocessor variables and include paths.  It also offers
guaranteed ordering to support SystemVerilog package dependence.

The second mode is source auto-discovery.  Pass a set of directories on
the command line and they will be searched for RTL sources.''')

parser.add_argument('dirlist', nargs='*',
                    help='list of directories to scan')
parser.add_argument('-s', '--sources',
                    help="""file containing list of source files.  The file
                            will be parsed by rtl_src_config.""")
parser.add_argument('-t', '--tool', choices=['VCS', 'QUESTA'], default=None,
                    help='simulator tool to use, default is VCS if present')
parser.add_argument('-p', '--platform',
                    default=get_default_platform(),
                    help='FPGA Platform to simulate')
parser.add_argument('-x', '--exclude', default=None,
                    help="""file name pattern to exclude
                            (auto-discovery mode only)""")

args = parser.parse_args()

if (len(args.dirlist) == 0) and not args.sources:
    parser.print_usage(sys.stderr)
    errorExit(
        "either --sources or at least on scan directory must be specified.  " +
        "See --help.")
if len(args.dirlist) and args.sources:
    parser.print_usage(sys.stderr)
    errorExit(
        "scan directories may not be specified along with --sources.  " +
        "See --help.")

tool_brand = args.tool
if (tool_brand is None):
    # Simulator wasn't specified.  Use VCS if it is present.
    try:
        with open(os.devnull, 'w') as fnull:
            subprocess.call(['vcs', '-ID'], stdout=fnull, stderr=fnull)
        tool_brand = 'VCS'
    except OSError:
        tool_brand = 'QUESTA'

# Write Makefile snippet #
fd = open("ase_sources.mk", "w")
# Print Information in ase_sources.mk
fd.write("##############################################################\n")
fd.write("#                                                            #\n")
fd.write("# Xeon(R) + FPGA AFU Simulation Environment                  #\n")
fd.write("# File generated by ase/scripts/generate_ase_environment.py  #\n")
fd.write("#                                                            #\n")
fd.write("##############################################################\n")
fd.write("\n")

# Update SIMULATOR
print("Tool Brand: " + tool_brand)
fd.write("SIMULATOR ?= ")
fd.write(tool_brand)
fd.write("\n\n")

# Configure RTL sources
if (args.sources):
    # Sources are specified explicitly in a file containing a list of sources
    json_file = config_sources(fd, args.sources)
else:
    # Discover sources by scanning a set of directories
    json_file = auto_find_sources(fd)

# Both source discovery methods may return a JSON file describing the AFU.
# They will return None if no JSON file is found.
gen_afu_platform_ifc(json_file)
if (json_file):
    gen_afu_json_verilog_macros(json_file)

ase_platform = get_ase_platform()
plat_type = {'intg_xeon': 'FPGA_PLATFORM_INTG_XEON',
             'discrete': 'FPGA_PLATFORM_DISCRETE'}.get(ase_platform)
print("ASE Platform: {0} ({1})".format(ase_platform, plat_type))

# Update ASE_PLATFORM
fd.write("ASE_PLATFORM ?= ")
fd.write(plat_type)
fd.write("\n\n")

fd.close()

# Write tool specific scripts.
#
# Scripts for multiple tools are written since the default tool
# can be selected at build time.

# Write Synopsys setup file & TCL script. Setup includes stub
# files that will hold lists of Quartus simulation libraries
# that will be populated by the ASE Makefile.
with open("synopsys_sim.setup", "w") as fd:
    fd.write("WORK > DEFAULT\n")
    fd.write("DEFAULT : ./work\n")
    fd.write("OTHERS = ./work/synopsys_sim_quartus_verilog.setup\n")
    fd.write("OTHERS = ./work/synopsys_sim_quartus_vhdl.setup\n")

open("vcs_run.tcl", "w").write('dump -depth 0 \ndump -aggregates -add /'
                               '\nrun \nquit\n')

# Generate .DO file
open("vsim_run.tcl", "w").write("add wave -r /* \nrun -all\n")
