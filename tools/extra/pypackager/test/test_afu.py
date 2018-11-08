# test_afu_platform.py
import unittest
import packager
import os
from afu import AFU

filepath = os.path.dirname(os.path.realpath(__file__))


class jsonTest(unittest.TestCase):
    def testGoodAFU(self):
        afu = AFU(filepath + "/test_data/good_afu_test.json")
        self.assertTrue(afu.validate())

    def testEmptyAFU(self):
        try:
            AFU(None)
        except Exception:
            self.fail(
                "AFU() should not throw exception if no AFU JSON is specified")

    def testBadAFU(self):
        self.assertRaises(
            Exception,
            AFU,
            filepath +
            "/test_data/bad_afu_test.json")


if __name__ == '__main__':
    unittest.main()
