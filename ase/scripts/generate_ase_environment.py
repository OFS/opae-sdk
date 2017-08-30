#!/usr/bin/env python
## Copyright(c) 2013-2017, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

#######################################################################
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
#######################################################################

# Future-proofing against Python 3 syntax changes in 'print'
from __future__ import print_function
import argparse
import ase_functions
import os, re, sys
from collections import defaultdict
from fnmatch import fnmatch

reload(sys)
sys.setdefaultencoding('utf8')

if sys.version_info < (2, 7):
    import commands
else:
    import subprocess

### Supported file extensions
# USERs may modify this if needed
VLOG_EXTENSIONS = [ ".svh", ".sv", ".vs", ".v" ]
VHD_EXTENSIONS = [ ".vhd", ".vhdl" ]

VHDL_FILE_LIST = os.environ['PWD'] + "/vhdl_files.list"
VLOG_FILE_LIST = os.environ['PWD'] + "/vlog_files.list"
TOOL_BRAND     = "VCS"

# Forbidden characters
SPECIAL_CHARS='\[]~!@#$%^&*(){}:;+$\''

##########################################################
###                                                    ###
###        DO NOT MODIFY BELOW THIS COMMENT BLOCK      ###
###                                                    ###
##########################################################
# Global variables
arg_list = []
tolowarg_list = []
valid_dirlist = []

special_chars_in_path = 0

def remove_dups(filepath, exclude=None):
    import hashlib
    include = lambda f : exclude is None or not fnmatch(f, exclude)
    files = []
    hashes = dict()
    with open(filepath, 'r') as fd:
        files = filter(lambda l : l.strip() != '', fd.readlines())
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
    return text



#################### Run command and get string output #######################
def commands_getoutput(cmd):
    if sys.version_info < (2,7):
        return commands.getoutput(cmd)
    else:
        byte_out = subprocess.check_output(cmd.split())
        str_out = byte_out.decode("utf-8")
        return str_out

############################# Print Help #####################################
def show_help():
    ase_functions.begin_red_fontcolor()
    print("INCORRECT command, CORRECT (required) usage is:")
    print("python generate_ase_environment.py <dir 1> [dir 2] ... [dir n] [-t <VCS|QUESTA>] [-p <FPGA_PLATFORM_INTG_XEON|FPGA_PLATFORM_DISCRETE>]")
    print("")
    print("Required switches => ")
    print("     <dir1>           => Atleast one sources directory is required")
    print("     [dir2]...[dir n] => Other optional directories with sources")
    print("")
    print("Optional switches => ")
    print("     -h,--help        => Show this help message")
    print("     -t,--tool        => Enter tool type as 'VCS' or 'QUESTA'")
    print("     -p,--plat        => Enter platform type as 'intg_xeon' or 'discrete'")
    print("")
    ase_functions.end_red_fontcolor()

########################### Has duplicates ###################################
def has_duplicates(word_dict):
    dups = filter(lambda (k,v) : len(v) > 1, word_dict.items())
    if dups:
        print("Duplicates found -")
        for k,v in dups:
            print(k)
            print('\n'.join(['\t{}'.format(l) for l in v]))
        return True
    return False

######################## Close file and exit #################################
def print_instructions():
    print("")
    ase_functions.begin_green_fontcolor()
    print("NOTES TO USER => ")
    print("* This script assumes File Extensions: ")
    print("  * VHDL : .vhd")
    print("  * V/SV : .sv .vs .v")
    print("  * If you use arbitrary extensions, please edit this script to reflect them, and re-run the script")
    print("* See ase_sources.mk and check for correctness")
    print("* See if DUT_INCDIR has all the locations mentioned")
    print("  * If a directory is missing, append it separated by '+' symbol")
    ase_functions.end_green_fontcolor()
    print("")

#############################################################################
##                        Script begins here                               ##
#############################################################################
print("#################################################################")
print("#                                                               #")
print("#             OPAE Intel(R) Xeon(R) + FPGA Library              #")
print("#               AFU Simulation Environment (ASE)                #")
print("#                                                               #")
print("#################################################################")
parser = argparse.ArgumentParser()
parser.add_argument('dirlist', nargs='+',
                    help='list of directories to scan')
parser.add_argument('-t', '--tool', choices=['VCS', 'QUESTA'], default='VCS',
                    help='simulator tool to use, default is VCS')
parser.add_argument('-p', '--plat', choices=['intg_xeon', 'discrete'], default='intg_xeon',
                    help='FPGA Platform to simulate')
parser.add_argument('-x', '--exclude', default=None,
                    help='file name pattern to exclude')
args = parser.parse_args()
tool_type = args.tool
TOOL_BRAND = args.tool
PLAT_TYPE = {'intg_xeon': 'FPGA_PLATFORM_INTG_XEON',
             'discrete' : 'FPGA_PLATFORM_DISCRETE'}.get(args.plat)
print("\nTool Brand : ", TOOL_BRAND)
print("\nPlatform Type : ", PLAT_TYPE)
#######################################################################
# Prepare list of candidate directories
print ("Valid directories supplied => "); 
valid_dirlist = filter(lambda p : os.path.exists(p), args.dirlist)
str_dirlist = " ".join(valid_dirlist)
if len(valid_dirlist) == 0:
    ase_functions.begin_red_fontcolor()
    print("No Valid source directories were specified ... please re-run script with legal directory name")
    show_help()
    ase_functions.end_red_fontcolor()
    sys.exit(0)

########################################################
### Write Makefile snippet ###
########################################################
fd = open("ase_sources.mk", "w")
# Print Information in ase_sources.mk
fd.write("##############################################################\n")
fd.write("#                                                            #\n")
fd.write("# Xeon(R) + FPGA AFU Simulation Environment                  #\n")
fd.write("# File generated by ase/scripts/generate_ase_environment.py  #\n")
fd.write("#                                                            #\n")
fd.write("##############################################################\n")
fd.write("\n")

########################################################
# Check if VHDL files exist, populate if any
########################################################
print("")
print("Finding VHDL files ... ")
str = ""
vhdl_filepaths = ""
for extn in VHD_EXTENSIONS:
    str = str + commands_getoutput("find -L " + str_dirlist + " -type f -name *" + extn)
    if len(str) != 0:
        str = str + "\n"
if len(str.strip()) != 0:
    open(VHDL_FILE_LIST, "w").write(str)
    vhdl_filepaths = str
    print("DUT_VHD_SRC_LIST = " + VHDL_FILE_LIST)
    fd.write("DUT_VHD_SRC_LIST = " + VHDL_FILE_LIST + " \n\n")
else:
    print("No VHDL files were found !")

########################################################
# Check if V/SV files exist, populate if any
########################################################
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
    cmd = "find -L " + str_dirlist + " -type f -name *" + extn + " -not -name *pkg*" + extn
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

########################################################
# Recursively find and add directory locations for VH
########################################################
print("")
print("Finding include directories ... ")
str = commands_getoutput("find -L " + str_dirlist + " -type d")
str = str.replace("\n", "+")
if len(str) != 0:
    print("DUT_INCDIR = " + str)
    fd.write("DUT_INCDIR = " + str + "\n\n")

#############################################
# Find ASE HW files ###
#############################################
pwd = commands_getoutput("pwd").strip()

#############################################
# Update SIMULATOR
#############################################
fd.write("SIMULATOR ?= ")
fd.write(TOOL_BRAND)
fd.write("\n\n")

#############################################
# Update ASE_PLATFORM
#############################################
fd.write("ASE_PLATFORM ?= ")
fd.write(PLAT_TYPE)
fd.write("\n\n")

fd.close()

#############################################
# Write tool specific scripts
#############################################
if tool_type == "VCS":
    print ("Generating VCS specific Runtime TCL scripts")
    ### Write Synopsys Setup file& TCL script ###
    open("synopsys_sim.setup", "w").write("WORK > DEFAULT\nDEFAULT : ./work\n")
    open("vcs_run.tcl", "w").write("dump -depth 0 \ndump -aggregates -add / \nrun \nquit\n")
elif tool_type == "QUESTA":
    ### Generate .DO file ###
    print ("Generating Modelsim specific scripts")
    open("vsim_run.tcl", "w").write("add wave -r /* \nrun -all\n")

#############################################
# Print special character message
#############################################
if (special_chars_in_path == 1):
    ase_functions.begin_red_fontcolor()
    print("Special characters found in path name --- RTL simulator tools may have trouble deciphering paths")
    ase_functions.end_red_fontcolor()

#############################################
# Module repetition check
#############################################
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

ase_functions.begin_red_fontcolor()
if (has_duplicates(module_files) == True):
    print("\n")
    print("Duplicate module names were found in the RTL file lists.")
    print("Please remove them manually as RTL compilation is expected to FAIL !")
ase_functions.end_red_fontcolor()

#############################################
# Print instructions
#############################################
print_instructions()
