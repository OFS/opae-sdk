import io
import unittest
from unittest import mock
from opae.admin.utils.utils import (_quote,
                                    dry_run_func,
                                    dry_run,
                                    max10_or_nios_version,
                                    get_fme_version,
                                    version_comparator,
                                    parse_timedelta)


class test_utils(unittest.TestCase):
    def setUp(self):
        self._data = 0xfedcba9844020108
    '''test__quote'''

    def test__quote(self):
        self.assertEquals(_quote('q'), '"q"')
        self.assertEquals(_quote('q/r', True), '"r"')
        self.assertEquals(_quote(1), 1)

    '''test_dry_run_func'''

    def test_dry_run_func(self):
        def mock_fn(*args):
            pass

        with mock.patch('sys.stdout', new=io.StringIO()) as _p:
            dry_run_func(mock_fn)(1, 2, 3)
            self.assertEquals(_p.getvalue(), 'mock_fn(1, 2, 3)\n')

    '''test_dry_run'''

    def test_dry_run(self):
        def mock_fn(*args):
            pass

        name = dry_run_func.__name__
        self.assertTrue(dry_run(mock_fn, True).__qualname__.startswith(name))
        self.assertEqual(dry_run(mock_fn, False), mock_fn)
        self.assertEqual(dry_run(0, True), 0)

    '''test_revision_get'''

    def test_max10_version(self):
        version_test = max10_or_nios_version(self._data)
        self.assertEquals(version_test.revision, 'D')
        self.assertEquals(version_test.major, 2)
        self.assertEquals(version_test.minor, 1)
        self.assertEquals(version_test.patch, 8)

        self.assertEquals(str(version_test), '2.1.8')
        assert version_test == max10_or_nios_version(self._data)
        assert version_test >= max10_or_nios_version(self._data-1)
        assert version_test <= max10_or_nios_version(self._data+1)
        assert version_test != max10_or_nios_version(0)

        self.assertEquals(repr(version_test), '2.1.8 Revision D ')


    '''test_get_fme_version'''

    def test_get_fme_version(self):
        self.assertEquals(
            get_fme_version((0x8086, 0x0b30), self._data),
            '{:d}.{:d}.{:d}'.format(0xf, 0xe, 0xd))

        with self.assertRaises(KeyError):
            get_fme_version((0x8086, 0x1234), self._data)

    '''test_to_int_tuple'''

    def test_to_int_tuple(self):
        self.assertEquals(version_comparator.to_int_tuple('1.2.3'), (1, 2, 3))

    '''test_label_get'''

    def test_label_get(self):
        init_expr = mock.MagicMock()
        version_comparator_test = version_comparator(init_expr)
        _ = version_comparator_test.label

    '''test_operator_get'''

    def test_operator_get(self):
        init_expr = mock.MagicMock()
        version_comparator_test = version_comparator(init_expr)
        _ = version_comparator_test.operator

    '''test_version_get'''

    def test_version_get(self):
        init_expr = mock.MagicMock()
        version_comparator_test = version_comparator(init_expr)
        _ = version_comparator_test.version

    '''test_compare'''

    def test_compare(self):
        version_comparator_test = version_comparator('foo < 1.2')
        version_comparator_test.parse()
        version_comparator_test.compare('1.1')

    '''test_parse_timedelta'''

    def test_parse_timedelta(self):
        parse_timedelta('1.5d')
