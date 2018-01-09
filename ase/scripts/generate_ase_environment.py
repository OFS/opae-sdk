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
import re
import sys
import subprocess
from collections import defaultdict
from fnmatch import fnmatch
import json
import subprocess

reload(sys)
sys.setdefaultencoding('utf8')

# Supported file extensions
# USERs may modify this if needed
VLOG_EXTENSIONS = [".svh", ".sv", ".vs", ".v"]
VHD_EXTENSIONS = [".vhd", ".vhdl"]

VHDL_FILE_LIST = os.getcwd() + "/vhdl_files.list"
VLOG_FILE_LIST = os.getcwd() + "/vlog_files.list"

# Forbidden characters
SPECIAL_CHARS = '\[]~!@#$%^&*(){}:;+$\''


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
        if exclude is None or not fnmatch(f, exclude):
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


# Run command and get string output #
def commands_getoutput(cmd):
    byte_out = subprocess.check_output(cmd.split())
    str_out = byte_out.decode()
    return str_out


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
            # + simulator commands go in both Verilog and VHDL
            vlog_srcs.append(s)
            # Unfortuantely, vhdlan (VCS VHDL simulator) doesn't support
            # the same directives as the Verilog simulator.
            if (tool_brand != 'VCS'):
                vhdl_srcs.append(s)
        elif (s[0] == '-'):
            # For now assume - is an include directive and used only for
            # Verilog. Escape all but the first space, which likely
            # follows a simulator command.
            spl = s.split(' ')
            if (len(spl) > 1):
                s = spl[0] + ' ' + '\ '.join(spl[1:])
            vlog_srcs.append(s)
            vlog_found = True
        else:
            # Convert extensions to lower case for comparison
            sl = s.lower()
            # Escape spaces in pathnames
            s = s.replace(' ', '\ ')

            # Verilog or SystemVerilog?
            for ext in VLOG_EXTENSIONS:
                if (sl[-len(ext):] == ext):
                    vlog_srcs.append(s)
                    vlog_found = True
                    break

            # VHDL?
            for ext in VHD_EXTENSIONS:
                if (sl[-len(ext):] == ext):
                    vhdl_srcs.append(s)
                    vhdl_found = True
                    break

            if (sl[-5:] == '.json'):
                json_srcs.append(s)

    # List Verilog & SystemVerilog sources in a file
    if (vlog_found):
        fd.write("DUT_VLOG_SRC_LIST = " + VLOG_FILE_LIST + " \n\n")
        with open(VLOG_FILE_LIST, "w") as f:
            for s in vlog_srcs:
                f.write(s + "\n")

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
        str = commands_list_getoutput(
            "find -L".split(" ") + [dir] +
            "-maxdepth 1 -type f -name *.json".split(" "))
        if (len(str)):
            # Use the discovered JSON file, but complain that it should
            # have been named explicitly.
            json_file = str.split('\n')[0]
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
        str = str + commands_getoutput("find -L " + str_dirlist +
                                       ' -type f -name *' + extn)
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
    vlog_filepaths = ""
    cmd = ""
    for extn in VLOG_EXTENSIONS:
        cmd = "find -L " + str_dirlist + " -type f -name *pkg*" + extn
        str = str + commands_getoutput(cmd)
        if len(str) != 0:
            str = str + "\n"
    for extn in VLOG_EXTENSIONS:
        cmd = "find -L " + str_dirlist + \
            " -type f -name *" + extn + " -not -name *pkg*" + extn
        str = str + commands_getoutput(cmd)
        if len(str) != 0:
            str = str + "\n"
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
    pathname = os.path.abspath(str_dirlist)
    str = commands_getoutput("find -L " + pathname + " -type d")

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
    str = commands_getoutput(
        "find -L " + str_dirlist + " -type f -name *.json")
    if (len(str)):
        for js in str.split('\n'):
            try:
                with open(js) as f:
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
        cfg.append("discrete_pcie3")
    else:
        cfg.append(args.platform)

    try:
        sys.stdout.write(commands_list_getoutput(cfg))
    except Exception:
        errorExit(cmd + " from OPAE SDK failed!")


# Generate a Verilog header file with values from the AFU JSON file
def gen_afu_json_verilog_macros(json_file):
    cmd = ['afu_json_mgr', 'json-info']
    cmd.append('--afu-json=' + json_file)
    cmd.append('--verilog-hdr=rtl/afu_json_info.vh')

    try:
        sys.stdout.write(commands_list_getoutput(cmd))
    except Exception:
        errorExit("afu_json_mgr from OPAE SDK failed, parsing {0}".format(
            json_file))


print("#################################################################")
print("#                                                               #")
print("#             OPAE Intel(R) Xeon(R) + FPGA Library              #")
print("#               AFU Simulation Environment (ASE)                #")
print("#                                                               #")
print("#################################################################")
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
parser.add_argument('-p', '--platform', choices=['intg_xeon', 'discrete'],
                    default='intg_xeon', help='FPGA Platform to simulate')
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

PLAT_TYPE = {'intg_xeon': 'FPGA_PLATFORM_INTG_XEON',
             'discrete': 'FPGA_PLATFORM_DISCRETE'}.get(args.platform)
print("\nTool Brand: ", tool_brand)
print("Platform Type: ", PLAT_TYPE)

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
fd.write("SIMULATOR ?= ")
fd.write(tool_brand)
fd.write("\n\n")

# Update ASE_PLATFORM
fd.write("ASE_PLATFORM ?= ")
fd.write(PLAT_TYPE)
fd.write("\n\n")

# Configure RTL sources
if (args.sources):
    # Sources are specified explicitly in a file containing a list of sources
    json_file = config_sources(fd, args.sources)
else:
    # Discover sources by scanning a set of directories
    json_file = auto_find_sources(fd)

fd.close()

# Both source discovery methods may return a JSON file describing the AFU.
# They will return None if no JSON file is found.
gen_afu_platform_ifc(json_file)
if (json_file):
    gen_afu_json_verilog_macros(json_file)

# Write tool specific scripts

# Scripts for multiple tools are written since the default tool
# can be selected at build time.

# Write Synopsys Setup file & TCL script
open("synopsys_sim.setup", "w").write("WORK > DEFAULT\nDEFAULT : ./work\n")
open("vcs_run.tcl", "w").write('dump -depth 0 \ndump -aggregates -add /'
                               '\nrun \nquit\n')

# Generate .DO file
open("vsim_run.tcl", "w").write("add wave -r /* \nrun -all\n")
