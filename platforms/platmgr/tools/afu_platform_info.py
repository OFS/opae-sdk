#!/usr/bin/env python3

#
# Copyright (c) 2018, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of the Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# This script extracts state from a platform database, mostly for use in
# other scripts.
#

import os
import sys
import argparse
import glob
import json

from platmgr.lib.jsondb import jsondb
from platmgr.db.info import plat_db_root


def errorExit(msg):
    sys.stderr.write("\nafu_platform_info error: " + msg + "\n")
    sys.exit(1)


def printKey(args, platform_db):
    # Default -- key matches a top-level db field exactly
    if ((args.key != '') and (args.key in platform_db)):
        print(platform_db[args.key])
        return

    if (args.key == 'fpga-family'):
        # Default value (A10) predicates fpga-family being present in the db.
        print('A10')
        return


#
# Return a list of all platform names found on the search path.
#
def findPlatforms(db_path):
    platforms = set()
    # Walk all the directories
    for db_dir in db_path:
        # Look for JSON files in each directory
        for json_file in glob.glob(os.path.join(db_dir, "*.json")):
            try:
                with open(json_file, 'r') as f:
                    # Does it have a platform name field?
                    db = json.load(f)
                    platforms.add(db['platform-name'])
            except Exception:
                # Give up on this file if there is any error
                None

    return sorted(list(platforms))


#
# Compute a directory search path given an environment variable name.
# The final entry on the path is set to default_dir.
#
def getSearchPath(env_name, default_dir):
    path = []

    if (env_name in os.environ):
        # Break path string using ':' and drop empty entries
        path = [p for p in os.environ[env_name].split(':') if p]

    # Append the database directory shipped with a release if
    # the release containts hw/lib/platform/<default_dir>.
    if ('OPAE_PLATFORM_ROOT' in os.environ):
        release_db_dir = os.path.join(os.environ['OPAE_PLATFORM_ROOT'],
                                      'hw', 'lib', 'platform',
                                      default_dir)
        if (os.path.isdir(release_db_dir)):
            path.append(release_db_dir)

    # Append the default directory from OPAE SDK
    path.append(os.path.join(plat_db_root, default_dir))

    return path


def main():
    # Users can extend the AFU and platform database search paths beyond
    # the OPAE SDK defaults using environment variables.
    afu_top_ifc_db_path = getSearchPath(
        'OPAE_AFU_TOP_IFC_DB_PATH', 'afu_top_ifc_db')
    platform_db_path = getSearchPath('OPAE_PLATFORM_DB_PATH', 'platform_db')

    msg = '''
afu_platform_info extracts configuration state from a platform database.
The search path for database files is configurable with an environment
variable using standard colon separation between paths:

Platform database directories (OPAE_PLATFORM_DB_PATH):
'''
    for p in platform_db_path[:-1]:
        msg += '  ' + p + '\n'
    msg += '  ' + platform_db_path[-1] + ' [default]\n'

    platform_names = findPlatforms(platform_db_path)
    if (platform_names):
        msg += "\n  Platforms found:\n"
        for p in platform_names:
            msg += '    ' + p + '\n'

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="Extract configuration state for a platform.",
        epilog=msg)

    # Positional arguments
    parser.add_argument(
        "platform",
        help="""Either the name of a platform or the name of a platform
                JSON file. If the argument is a platform name, the
                platform JSON file will be loaded from the platform
                database directory search path (see below).""")

    parser.add_argument(
        "-k", "--key",
        choices=["ase-platform", "fpga-family"],
        help="""State to fetch.""")

    # Verbose/quiet
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "-v", "--verbose", action="store_true", help="Verbose output")
    group.add_argument(
        "-q", "--quiet", default=True,
        action="store_true", help="Reduce output")
    args = parser.parse_args()

    # Load the platform database
    platform = jsondb(args.platform, platform_db_path, 'platform', args.quiet)
    platform.canonicalize()

    # Load the platform default parameters
    platform_defaults = jsondb('platform_defaults', platform_db_path,
                               'platform-params', args.quiet)
    platform_defaults.canonicalize()

    printKey(args, platform.db)


if __name__ == "__main__":
    main()
