import errno
import io
import json
import hashlib
import struct
import sys
import tempfile
import unittest
import uuid
from unittest import mock
from opae.admin.tools.fpgasupdate import (decode_gbs_header,
                                          do_partial_reconf,
                                          decode_auth_block0,
                                          canonicalize_bdf,
                                          main,
                                          SecureUpdateError,
                                          update_fw,
                                          sig_handler,
                                          VALID_GBS_GUID,
                                          BLOCK0_SUBTYPE_CANCELLATION,
                                          DEFAULT_BDF)
from opae.admin.tools import fpgasupdate
from opae.admin.fpga import fpga


class fpgasupdate_fixture(unittest.TestCase):
    def setUp(self):
        self._device = fpga.enum()[0]
        self._secure_dev = open(self._device.secure_dev.devpath, 'w+')
        self._dummy_infile = tempfile.NamedTemporaryFile('wb+')
        with open('/dev/urandom', 'rb') as fp:
            self._dummy_infile.write(fp.read(4096))
        self._dummy_infile.seek(0)

        self._block0_size = 128
        self._block0_magic = 0xb6eafd19
        self._hash256 = hashlib.sha3_256()
        self._hash384 = hashlib.sha3_384()

        self._dummy_unsigned_blocks = io.BytesIO()
        self._dummy_unsigned_blocks.write(
            struct.pack('IIII',
                        self._block0_magic,
                        4096,
                        BLOCK0_SUBTYPE_CANCELLATION,
                        0))
        self._dummy_unsigned_blocks.write(self._hash256.digest())
        self._dummy_unsigned_blocks.write(self._hash384.digest())
        self._dummy_unsigned_blocks.write(struct.pack('I', 0))

    def tearDown(self):
        self._secure_dev.close()
        self._dummy_infile.close()
        # os.unlink(self._secure_dev.name)


class test_fpgasupdate(fpgasupdate_fixture):

    '''test_decode_gbs_header'''

    def test_decode_gbs_header(self):
        self._dummy_infile.truncate(8)
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        self.assertIsNone(decode_gbs_header(self._dummy_infile))

        _uuid = uuid.uuid4()
        self._dummy_infile.truncate(20)
        self._dummy_infile.seek(0)
        self._dummy_infile.write(bytes(reversed(_uuid.bytes)))
        self._dummy_infile.write(bytes([0, 0]))
        self._dummy_infile.seek(0)
        self.assertIsNone(decode_gbs_header(self._dummy_infile))

        interface_uuid = '01234567-89AB-CDEF-0123-456789ABCDEF'
        metadata = {'clock-frequency': 31.2,
                    'power': 50,
                    'interface-uuid': interface_uuid,
                    'magic-no': 488605312,
                    'accelerator-clusters': [{
                        'total-contexts': 1,
                        'name': 'nlb_400',
                        'accelerator-type-uuid':
                            'd8424dc4-a4a3-c413-f89e-433683f9040b'
                    }]}
        metadata_bytes = json.dumps(metadata).encode()
        self._dummy_infile.seek(0)
        self._dummy_infile.write(VALID_GBS_GUID.bytes)
        self._dummy_infile.write(struct.pack('I', len(metadata_bytes)))
        self._dummy_infile.write(metadata_bytes)
        self._dummy_infile.seek(0)
        self.assertIsNone(decode_gbs_header(self._dummy_infile))

        metadata['afu-image'] = metadata.pop('accelerator-clusters')[0]
        metadata_bytes = json.dumps(metadata).encode()
        self._dummy_infile.seek(0)
        self._dummy_infile.write(VALID_GBS_GUID.bytes)
        self._dummy_infile.write(struct.pack('I', len(metadata_bytes)))
        self._dummy_infile.write(metadata_bytes)
        self._dummy_infile.seek(0)
        self.assertIsNone(decode_gbs_header(self._dummy_infile))

        metadata['afu-image']['interface-uuid'] = metadata.pop(
            'interface-uuid')
        metadata_bytes = json.dumps(metadata).encode()
        self._dummy_infile.seek(0)
        self._dummy_infile.write(VALID_GBS_GUID.bytes)
        self._dummy_infile.write(struct.pack('I', len(metadata_bytes)))
        self._dummy_infile.write(metadata_bytes)
        self._dummy_infile.seek(0)
        result = decode_gbs_header(self._dummy_infile)
        self.assertIsNotNone(result)
        self.maxDiff = None
        pretty_guid_bytes = 'XeonFPGA?GBSv001'
        self.assertDictEqual(result,
                             {'valid_gbs_guid': VALID_GBS_GUID,
                              'pretty_gbs_guid': pretty_guid_bytes,
                              'metadata': metadata_bytes,
                              'interface-uuid': interface_uuid})

    '''test_do_partial_reconf'''

    def test_do_partial_reconf(self):
        do_partial_reconf_addr = self._device.pci_node.pci_address
        do_partial_reconf_filename = self._dummy_infile.name
        with mock.patch('subprocess.check_output', return_value='success'):
            do_partial_reconf(do_partial_reconf_addr,
                              do_partial_reconf_filename)

    '''test_decode_auth_block0'''

    def test_decode_auth_block0(self):
        # invalid file size
        self._dummy_infile.truncate(8)
        self._dummy_infile.seek(0)
        self.assertIsNone(decode_auth_block0(self._dummy_infile, False))

        # make the file size valid
        self._dummy_infile.truncate(self._block0_size + 896 + 128)

        # valid magic, prompt off
        self._dummy_infile.seek(0)
        self._dummy_infile.write(self._dummy_unsigned_blocks.getvalue())
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        self.assertIsNotNone(decode_auth_block0(self._dummy_infile, False))

        # valid magic, prompt on
        def _input(prompt, *args, **kwargs):
            assert prompt.startswith('*** P')
            return 'yes\n'

        with mock.patch.object(fpgasupdate, 'input', new=_input):
            self.assertIsNotNone(
                decode_auth_block0(self._dummy_infile, True))

        # invalid magic
        self._dummy_infile.seek(0)
        self._dummy_unsigned_blocks.seek(0)
        self._dummy_unsigned_blocks.write(struct.pack('I', 0))
        self._dummy_infile.write(self._dummy_unsigned_blocks.getvalue())
        self._dummy_infile.seek(0)
        self.assertIsNone(decode_auth_block0(self._dummy_infile, False))

    '''test_canonicalize_bdf'''

    def test_canonicalize_bdf(self):
        canonicalize_bdf_bdf = self._device.pci_node.pci_address
        canonicalize_bdf(canonicalize_bdf_bdf)
        canonicalize_bdf_bdf = self._device.pci_node.bdf
        canonicalize_bdf(canonicalize_bdf_bdf)

    '''test_update_fw'''

    def test_update_fw(self):
        update_fw_fd_dev = self._secure_dev
        update_fw_infile = self._dummy_infile
        with mock.patch('fcntl.ioctl'):
            update_fw(update_fw_fd_dev, update_fw_infile)

    '''test_update_fw_fail'''

    def test_update_fw_fail(self):
        update_fw_fd_dev = self._secure_dev
        update_fw_infile = self._dummy_infile
        with mock.patch('fcntl.ioctl', side_effect=IOError):
            update_fw(update_fw_fd_dev, update_fw_infile)

        err = IOError()
        err.errno = errno.EAGAIN
        side_effect = [err, mock.DEFAULT]
        for se in [side_effect,
                   side_effect*2,
                   [mock.DEFAULT]*3+[err, IOError()]]:
            update_fw_infile.seek(0)
            with mock.patch('fcntl.ioctl', side_effect=se):
                try:
                    update_fw(update_fw_fd_dev, update_fw_infile)
                except StopIteration:
                    pass

        update_fw_infile.seek(0)
        with mock.patch('fcntl.ioctl', side_effect=[err, mock.DEFAULT, err]):
            try:
                update_fw(update_fw_fd_dev, update_fw_infile)
            except StopIteration:
                pass

    '''test_sig_handler'''

    def test_sig_handler(self):
        sig_handler_signum = 1
        sig_handler_frame = None
        with self.assertRaises(SecureUpdateError):
            sig_handler(sig_handler_signum, sig_handler_frame)


'''test_main'''
@mock.patch('signal.signal')
class test_fpgasupdate_main(fpgasupdate_fixture):
    def test_main(self, _signal):
        self.skipTest('testing...')
        argv = ['fpgasupdate',
                self._dummy_infile.name,
                self._device.pci_node.bdf,
                '-y']
        self._dummy_infile.seek(0)
        self._dummy_infile.write(self._dummy_unsigned_blocks.getvalue())
        self._dummy_infile.flush()
        self._dummy_infile.seek(0)
        with mock.patch.object(sys, 'argv', new=argv):
            with mock.patch('fcntl.ioctl'):
                with self.assertRaises(SystemExit) as se:
                    main()
                    self.assertEquals(se.returncode, 0)

        self._dummy_infile.seek(0)
        with mock.patch.object(fpgasupdate, 'update_fw',
                               side_effect=SecureUpdateError((1, 'fail'))):
            with mock.patch.object(sys, 'argv', new=argv):
                with mock.patch('fcntl.ioctl'):
                    with self.assertRaises(SystemExit) as se:
                        main()
                        self.assertEquals(se.returncode, 1)

        self._dummy_infile.seek(0)
        with mock.patch.object(fpgasupdate, 'update_fw',
                               side_effect=KeyboardInterrupt()):
            with mock.patch.object(sys, 'argv', new=argv):
                with mock.patch('fcntl.ioctl'):
                    with self.assertRaises(SystemExit) as se:
                        main()
                        self.assertEquals(se.returncode, 1)

        argv[2] = DEFAULT_BDF
        self._dummy_infile.seek(0)
        with mock.patch.object(sys, 'argv', new=argv):
            with self.assertRaises(SystemExit) as se:
                main()
                self.assertEquals(se.returncode, 1)

        with mock.patch.object(sys, 'argv', new=argv):
            with mock.patch('fcntl.ioctl'):
                with self.assertRaises(SystemExit) as se:
                    main()
