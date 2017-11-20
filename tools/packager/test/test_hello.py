import unittest


class HelloWorldTestCase(unittest.TestCase):
    def runTest(self):
        import packager
        self.assertTrue('main' in dir(packager))


if __name__ == '__main__':
    unittest.main()
