# Copyright(c) 2017, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#  this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
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

import json
import os
import shutil
import sys
import utils
import zipfile
from metadata import metadata
from gbs import GBS, GBS_EXT

# Update sys.path to include jsonschema folder from different locations
try:
    # pkgPATH1 : jsonschema search path for opae-sdk/tools/extra/packager
    pkgPath1 = os.path.join(sys.path[0], 'jsonschema-2.3.0')

    # pkgPath2 : current packager script location
    pkgPath2 = os.path.abspath(os.path.dirname(sys.argv[0]))
    dirList = pkgPath2.split("/")
    dirList = dirList[:-1]
    pkgPath2 = "/".join(dirList)

    # pkgPath3 : jsonschema search path for current packager location
    pkgPath3 = pkgPath2 + "/share/opae/python/jsonschema-2.3.0"

    sys.path.append(pkgPath1)
    sys.path.append(pkgPath3)
    from jsonschema import validators
    from jsonschema import exceptions
except ImportError:
    print("jsonschema module has no validatiors() or exceptions()")
    raise

filepath = os.path.dirname(os.path.realpath(__file__))
schema_path = "schema/afu_schema_v01.json"
if(zipfile.is_zipfile(filepath)):
    archive = zipfile.ZipFile(filepath, 'r')
    afu_schema = json.load(archive.open(schema_path, "r"))
else:
    afu_schema = json.load(open(filepath + "/" + schema_path, "r"))

ARCHIVE_FORMAT = "zip"
ARCHIVE_EXT = ".zip"


class AFU(object):
    def __init__(self, afu_desc_file=None):
        self.afu_json = {}
        self.metadata_len = 0
        self.afu_desc_file = afu_desc_file
        if afu_desc_file:
            self.load_afu_desc_file(afu_desc_file)

    @classmethod
    def create_afu_from_gbs(cls, gbs):
        afu = cls()

        afu.afu_json = gbs.gbs_info
        afu.metadata_len = gbs.metadata_len

        return afu

    def load_afu_desc_file(self, afu_desc_file):
        if os.path.exists(afu_desc_file):
            self.afu_desc_file = os.path.abspath(afu_desc_file)
            self.afu_dir = os.path.dirname(afu_desc_file)
        else:
            raise Exception("Cannot find {0}".format(afu_desc_file))

        self.afu_json = json.load(open(self.afu_desc_file, "r"))
        self.compat_update()

        if not self.validate():
            raise Exception("Accelerator description file failed validation!")

    # Load AFU JSON file given an open file handle
    def load_afu_desc_file_hdl(self, afu_desc_file_hdl):
        self.afu_json = json.load(afu_desc_file_hdl)
        self.compat_update()

        if not self.validate():
            raise Exception("Accelerator description file failed validation!")

    # Update/rename fields as needed to maintain backward compatibility
    def compat_update(self):
        try:
            afu_ifc = self.afu_json['afu-image']['afu-top-interface']
            # The interface 'class' used to be called 'name'.
            # Maintain compatibility with older AFUs.
            if ('name' in afu_ifc):
                afu_ifc['class'] = afu_ifc.pop('name')
        except KeyError as e:
            None

    def validate(self, packaging=False):
        if self.afu_json == {}:
            return False
        try:
            validators.validate(self.afu_json, afu_schema)
        except exceptions.ValidationError as ve:
            print("JSON schema error at {0}: {1}".format(
                str(list(ve.path)), str(ve.message)))
            return False

        # If emitting a GBS file do some extra validation beyond the schema.
        if packaging:
            # User clocks can be "auto" in the source JSON in order to
            # set the frequency to the actual achieved speed.  When
            # creating the GBS, the frequencies must be numbers.
            for clock in ['clock-frequency-high', 'clock-frequency-low']:
                if clock in self.afu_json['afu-image']:
                    f = self.afu_json['afu-image'][clock]
                    if not isinstance(f, (int, float)):
                        print("JSON schema error at {0}: {1}").format(
                            "afu-image/" + clock, "expected number")
                        raise Exception("Accelerator description file " +
                                        "failed validation!")

        return True

    def update_afu_json(self, key_values):
        try:
            for value in key_values:
                # Colon separates key and value
                curr_val = value.split(':')
                curr_val[1] = utils.convert_to_native_type(curr_val[1])
                if self.afu_json:
                    # Compatibility support for old scripts that set
                    # interface-uuid, assuming it would be found in
                    # afu-image.  After all scripts are updated, this
                    # check can be removed.
                    if curr_val[0] == 'interface-uuid':
                        curr_val[0] = 'afu-image/interface-uuid'

                    # Slash separates key hierarchy
                    key = curr_val[0].split('/')
                    if len(key) > 1:
                        # Walk key hierarchy
                        afu = self.afu_json
                        for k in key[:-1]:
                            if k not in afu:
                                # Intermediate key not present, add it.
                                afu[k] = dict()
                            afu = afu[k]

                        # Add the new value
                        afu[key[-1]] = curr_val[1]
                    else:
                        # Old method didn't support key hierarchy.  Search
                        # for the key either in afu-image or at top-level.
                        # If not found, assume top-level.
                        if key[0] in self.afu_json:
                            self.afu_json[key[0]] = curr_val[1]
                        elif key[0] in self.afu_json['afu-image']:
                            self.afu_json['afu-image'][key[0]] = curr_val[1]
                        else:
                            self.afu_json[key[0]] = curr_val[1]
        except IndexError as e:
            print(e)
            raise Exception(
                "Invalid <key>:<value> pair passed using --set-value")

        if not self.validate():
            raise Exception(
                'AFU metadata validation failed after updating metadata '
                ' with values provided with --set_value')

    def create_gbs(self, rbf_file, gbs_file, key_values=None):
        if key_values:
            self.update_afu_json(key_values)

        # Set the expected magic number if it hasn't already been set
        if 'magic-no' not in self.afu_json['afu-image']:
            self.afu_json['afu-image']['magic-no'] = 0x1d1f8680

        self.validate(packaging=True)
        gbs = GBS.create_gbs_from_afu_info(rbf_file, self.afu_json)
        return gbs.write_gbs(gbs_file)

    # Dump AFU JSON to string
    def dumps(self):
        return json.dumps(self.afu_json, indent=3)

    def package(self, rbf_file, sw_dir, doc_dir, package_name):
        image_dir = os.path.join(utils.get_work_dir(), "image_0")
        if not os.path.exists(image_dir):
            os.makedirs(image_dir)

        gbs_name = os.path.splitext(
            os.path.basename(
                self.afu_desc_file))[0] + GBS_EXT
        gbs_path = os.path.join(image_dir, gbs_name)
        self.create_gbs(rbf_file, gbs_path)

        shutil.copyfile(
            self.afu_desc_file, os.path.join(
                image_dir, os.path.basename(
                    self.afu_desc_file)))

        package_dir = os.path.join(utils.get_work_dir(), "package")
        shutil.make_archive(
            os.path.join(
                package_dir,
                "image_0"),
            ARCHIVE_FORMAT,
            image_dir)
        if sw_dir:
            shutil.make_archive(
                os.path.join(
                    package_dir,
                    "sw"),
                ARCHIVE_FORMAT,
                sw_dir)
        if doc_dir:
            shutil.make_archive(
                os.path.join(
                    package_dir,
                    "docs"),
                ARCHIVE_FORMAT,
                doc_dir)

        shutil.make_archive(package_name, ARCHIVE_FORMAT, package_dir)
        shutil.rmtree(utils.get_work_dir())
