import unittest
import packager
import filecmp
import metadata.constants
import struct
import os
import json
from collections import OrderedDict

filepath = os.path.dirname(os.path.realpath(__file__))

afu_json = (
    '{"version": 1, "afu-image": {"clock-frequency-low": 50, '
    '"slot-type-uuid": "166b785b-0a5d-411a-bc2d-5b3d17c479a8"'
    ', "clock-frequency-high": 200, "accelerator-clusters": '
    '[{"total-contexts": 1, "accelerator-type-uuid": '
    '"c000c966-0d82-4272-9aef-fe5f84570612"'
    ', "name": "nlb_400"}], "power": 10}}')

expected_metadata = ("XeonFPGA" + chr(0xb7) + "GBSv001" +
                     struct.pack('<I', len(afu_json)) +
                     afu_json)

expected_no_json_metadata = ("XeonFPGA" + chr(0xb7) + "GBSv001" +
                             struct.pack('<I', 0))


""" Unit tests for the metadata.py file
"""


class TestMetadata(unittest.TestCase):

    def setUp(self):
        self.afu_json = json.loads(afu_json, object_pairs_hook=OrderedDict)

    # compare generated and expected metadata
    def test_get_metadata(self):
        self.assertTrue(
            list(expected_metadata) == metadata.metadata.get_metadata(
                self.afu_json))

    # test the case where there is no JSON
    def test_no_json(self):
        self.assertTrue(list(expected_no_json_metadata) ==
                        metadata.metadata.get_metadata({}))


if __name__ == '__main__':
    unittest.main()
