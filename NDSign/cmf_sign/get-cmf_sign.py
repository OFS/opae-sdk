#!/usr/bin/env python

import os
import sys
import subprocess
import re
import argparse
import shutil
import string

sys.path.append("./pefile-lib")
import pefile

def clean_cwd(verbose=False):

    list_clean = []

    for item in os.listdir("."):
        match = re.search(".(exe|dll)$", item, re.IGNORECASE)
        if match is not None:
            list_clean += [ item ]

    if len(list_clean) > 0:
        print "Found old exe or dll: "
        for item in list_clean:
            print " - " + item

        print
        confirm = raw_input("Clean old files? (y/n): ")
        print

        if confirm != "y" and confirm != "Y":
            print "Not cleaning. Goodbye!"
            print
            exit(0)

        print "Cleaning ..."
        print

    for item in list_clean:
        os.remove(item)

def dll_strings(pe_file, search_dirs=None):

    list_import_raw      = []
    list_import_filtered = []

    dll = pefile.PE(pe_file)

    if hasattr(dll, 'DIRECTORY_ENTRY_IMPORT'):
        for module in dll.DIRECTORY_ENTRY_IMPORT:
            list_import_raw += [ dll.get_string_at_rva(module.struct.Name) ]

    # for module in dll.DIRECTORY_ENTRY_BOUND_IMPORT:
    #     list_import_raw += [ dll.get_string_at_rva(module.struct.Name) ]
    #     pass
    # for module in dll.DIRECTORY_ENTRY_DELAY_IMPORT:
    #     list_import_raw += [ dll.get_string_at_rva(module.struct.Name) ]
    #     pass

    #
    # Clean the DLL list
    #

    for dll in list_import_raw:

        # if os.name != 'nt':
        #     if re.match("(kernel32)\\.dll", dll, re.IGNORECASE)
        #         continue

        if search_dirs is not None:
            if type(search_dirs) is str:
                search_dirs = ( search_dirs, )
            for dir in search_dirs:
                if os.path.isfile(dir + "/" + dll):
                    list_import_filtered += [ dll ]
                    break

    del dll

    return list_import_filtered

def dll_search_copy(search_file, search_path):

    search_file = search_file.lower()

    master_list = [ search_file ]

    for dll in dll_strings(search_file, search_path):

        dll = dll.lower()

        dll_search_copy_helper(master_list, search_file, dll, search_path)

    return sorted(master_list)

def dll_search_copy_helper(master_list, search_parent, search_file, search_path):

    print "Searching %s -> %s [%u]" % (search_parent, search_file, len(master_list))

    # If file does not already exist in this directory:
    if not os.path.isfile(search_file):

        copy_path = search_path + "/" + search_file

        if not os.path.isfile(copy_path):
            raise RuntimeError("Unable to locate file [%s]." % (copy_path))

        print "Copying %s" % (copy_path)
        shutil.copy(copy_path, ".")

    else:
        print "Existing %s" % (search_file)

    master_list.append(search_file)

    for dll in dll_strings(search_file, search_path):

        dll = dll.lower()

        if dll in master_list:
            continue

        dll_search_copy_helper(master_list, search_file, dll, search_path)

def main():

    default_release    = "19.1"
    default_build      = "current.windows64.release"  # Matches the symlink in the quartuskit dir.

    parser = argparse.ArgumentParser(description="Attempts to get cmf_sign.exe and DLLs needed to satisfy all " +
                                                 "dependencies. On Windows, the PICE or Legacy drives must be " +
                                                 "mapped to expected location, p: or s: respectively. This script " +
                                                 "is not foolproof. See https://hsdes.intel.com/resource/1408565970.",
                                     epilog="Author: Fred Hsueh (fred.hsueh@intel.com)")

    parser.add_argument("--release",    default=default_release,    help="QuartusKit version to use. Defaults to %s." % (default_release))
    parser.add_argument("--build",      default=default_build,      help="Build to use. Defaults to the latest build.")
    parser.add_argument("--clean",   action='store_true', help="Deletes existing DLL and EXE files.")
    # parser.add_argument("--verbose", action='store_true', help="Be more verbose.")

    args = parser.parse_args()

    path_quartus = ""

    if os.name == 'nt':
        if os.path.isdir("p:/"):
            print "Using PICE p:/ as file source."
            path_quartus = "p:/psg/swip/releases/quartuskit/" + args.release + "/" + args.build + "/windows64/quartus"

        elif os.path.isdir("s:/"):
            print "Using Legacy s:/ as file source."
            path_quartus = "s:/tools/quartuskit/" + args.release + "/" + args.build + "/windows64/quartus"

        else:
            raise RuntimeError("Windows network mount p: and s: does not exist.")

    else:
        arc_site = os.environ['ARC_SITE']

        if arc_site == "sc":
            print "Using PICE /p/psg/swip as file source."
            path_quartus = "/p/psg/swip/releases/quartuskit/" + args.release + "/" + args.build + "/windows64/quartus"

        elif arc_site == "sj":
            print "Using Legacy /tools as file source."
            path_quartus = "/tools/quartuskit/" + args.release + "/" + args.build + "/windows64/quartus"

        else:
            raise RuntimeError("Unknown ARC site (sorry Penang people! Feel free to fix this.)")

    path_quartus_bin64 = path_quartus + "/bin64"

    #
    # first remove any .exe and .dll file in the current directory
    #
    if args.clean:
        clean_cwd()

    #
    # Copy cmf_sign.exe to the cwd
    #

    if not os.path.isfile("cmf_sign.exe"):
        shutil.copy(path_quartus_bin64 + "/cmf_sign.exe", ".")

    #
    # Find strings in cmf_sign.exe that corresponds to a DLL and add to list
    #
    dll_search_copy("cmf_sign.exe", path_quartus_bin64)

    #
    # Warning for https://hsdes.intel.com/resource/1408565970
    # Message about missing msvcp140.dll and vcruntime140.dll
    #
    if not os.path.isfile("msvcp140.dll") or not os.path.isfile("vcruntime140.dll"):
        print
        print "Runtime files (msvcp140.dll and vcruntime140.dll) not present."
        print "cmf_sign.exe may not run correctly. If this is the case, find those"
        print "files in archive/msvcrt-14.13.zip and try again."
        print
        print "See https://hsdes.intel.com/resource/1408565970"

if __name__ == '__main__':
    main()
