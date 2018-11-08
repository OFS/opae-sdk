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

import os
import json
import struct
from metadata import constants
from metadata import metadata

RBF_EXT = ".rbf"
GBS_EXT = ".gbs"

"""
Class GBS for operations related to GBS files
"""


class GBS:
    def __init__(self, gbs_file=None):
        self.guid = ''
        self.metadata_len = 0
        self.gbs_info = ''
        self.rbf = ''
        self.metadata = []

        if gbs_file:
            self.filename = os.path.splitext(os.path.basename(gbs_file))[0]
            self.validate_gbs_file(gbs_file)

    """
    classmethod to create a gbs instance from json and
    rbf file. Used to create a new gbs file

    @return instance of the new GBS object
    """
    @classmethod
    def create_gbs_from_afu_info(cls, rbf_file, afu_json):
        gbs = cls()

        rbf = open(rbf_file, 'rb')
        rbf_content = rbf.read()

        gbs.guid = constants.METADATA_GUID
        gbs.metadata_len = len(afu_json)
        gbs.gbs_info = afu_json
        gbs.rbf = rbf_content
        gbs.metadata = metadata.get_metadata(afu_json)
        gbs.filename = os.path.splitext(os.path.basename(rbf_file))[0]

        return gbs

    """
    Set of get methods to retrieve gbs attributes
    """

    def get_gbs_guid(self):
        return self.guid

    def get_gbs_meta_len(self):
        return self.metadata_len

    def get_gbs_info(self):
        return self.gbs_info

    def get_rbf_val(self):
        return self.rbf

    def get_gbs_metadata(self):
        return self.metadata

    """
    Function to print GBS info to the console
    """

    def print_gbs_info(self):
        if self.gbs_info == '':
            raise Exception("No metadata in GBS file")

        print(json.dumps(self.gbs_info, indent=4))

    """
    Function to write a new rbf file to the filesystem
    """

    def write_rbf(self, rbf_file):
        if not rbf_file:
            rbf_file = self.filename + RBF_EXT

        with open(rbf_file, 'wb') as rbf:
            rbf.write(self.rbf)

        return rbf_file

    """
    Function to write a new gbs file to the filesystem
    """

    def write_gbs(self, gbs_file):
        if not gbs_file:
            gbs_file = self.filename + GBS_EXT

        gbs_file_header = bytearray(self.get_gbs_metadata())

        with open(gbs_file, 'wb') as gbs:
            gbs.write(gbs_file_header + self.rbf)

        return gbs_file

    """
    Function to update gbs info in an object with input info
    """

    def update_gbs_info(self, gbs_info):
        self.gbs_info = gbs_info
        self.metadata = metadata.get_metadata(self.gbs_info)

    """
    Function to make make sure GBS file conforms to standard
    and polpulate the GBS object with appropriate values
    """

    def validate_gbs_file(self, gbs_file):
        file = open(gbs_file, 'rb')
        gbs = file.read()

        if len(constants.METADATA_GUID) >= len(gbs):
            raise Exception("Invalid GBS file")

        self.guid = gbs[:constants.GUID_LEN]
        if self.guid != constants.METADATA_GUID:
            raise Exception("Unsupported GBS format")

        metadata_index = constants.GUID_LEN + constants.SIZEOF_LEN_FIELD

        metadata_len = struct.unpack(
            "<I", gbs[constants.GUID_LEN:metadata_index])
        self.metadata_len = metadata_len[0]

        if self.metadata_len != 0:
            self.gbs_info = json.loads(
                gbs[metadata_index:(metadata_index + self.metadata_len)])

        rbf_index = metadata_index + metadata_len[0]

        if rbf_index == len(gbs):
            raise Exception("No RBF in GBS file!")

        self.rbf = gbs[rbf_index:]

        self.metadata = metadata.get_metadata(self.gbs_info)
