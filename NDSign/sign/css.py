#!/usr/bin/env python

import copy
import getpass
import os
from threading import Lock
import unittest
import re
from base64 import b64encode

from bitstream.cmf import Cmf
from bitstream.engineering_cert import EngineeringCert
from bitstream.fragment_collection import BitstreamFragmentCollection
from bitstream.key import EcdsaPublicKey
from bitstream.signature import Signature
from bitstream.signature_chain import SignatureChain
from bitstream.signature_chain_block0_entry import SignatureChainBlock0Entry
from bitstream.signature_chain_root_entry import SignatureChainRootEntry
from bitstream.signature_chain_multi_root_entry import SignatureChainMultiRootEntry
from bitstream.zip import BitstreamZip
from sign.module import CmfDescriptorCssModule, CssModule, PublicKeyCssModule, EngineeringCertHeaderCssModule
from sign.cmfsign import CmfSign
from util.exttool import ExtTool
from util.fileutil import FileUtils
from util.mkdir import Mkdir
from bitstream.signature_chain_public_key_entry import SignatureChainPublicKeyEntry

class Css(ExtTool):

    css_exe_lock = Lock()

    def __init__(self):
        super(Css, self).__init__()
        self.uid = None
        self.sign_keychain_file = None
        self.sign_keypair_file = None
        self.permission = 0xFFFFFFFF
        self.cancel_id = 0
        self.debug = False
        self.verbose = False
        self.work_dir = None
        self.use_code_sign_server = False
        self.css_project = None
        self.css_secure_token_password = None
        self.css_secure_token_cert_keyhash = None
        self.standard_approve_roles = ["reviewer", "manager", "security"]

        # "debug" here really means you're using the test CSS server rather than production (See tests.mk "test-mode" target)
        # These are the reviewers that will be used for a debug sign using CSS, so they need to match the actual slots
        # for the person running the test (which will always be a MANUAL_TESTCASE in tests.mk). Since this cannot be known here,
        # an environment variable is used. Note that "reviewer" always matches the lowest reviewer slot you have, so
        # only REVIEWER2 and REVIEWER3 need to be set (for instance, on PSGTest2, jdasilva is reviewers 1, 2, and 3 (indexing starts at 1) when you run
        # codesign.exe --viewrolesproject=PSGTest2 so that's reviewer, reviewer2, reviewer3 with reviewer automatically mapped to reviewer1.
        # acopelan is slots 8, 9, and 10 so reviewer, reviewer9, reviewer10 will work with reviewer automatically mapped to reviewer8)
        # This doesn't come up for production because duplicate slots are not allowed (you can't fill the spot for multiple reviewers)
        self.debug_approve_roles = ["manager", "security", os.getenv("REVIEWER",  "reviewer"),
                                    os.getenv("REVIEWER2", "reviewer2"),
                                    os.getenv("REVIEWER3", "reviewer3")]

    def execute(self, cmd, verbose=True, cmd_echo=None, expected_return_code=0):
        Css.css_exe_lock.acquire()
        try:
            return super(Css, self).execute(cmd=cmd, verbose=verbose, cmd_echo=cmd_echo, expected_return_code=expected_return_code)
        finally:
            Css.css_exe_lock.release()

    def copy_settings(self, css):
        self.sign_keychain_file = css.sign_keychain_file
        self.sign_keypair_file = css.sign_keypair_file
        self.permission = css.permission
        self.cancel_id = css.cancel_id
        self.debug = css.cancel_id
        self.verbose = css.verbose
        self.work_dir = css.work_dir
        self.use_code_sign_server = css.use_code_sign_server
        self.css_project = css.css_project
        self.css_secure_token_password = css.css_secure_token_password
        self.css_secure_token_cert_keyhash = css.css_secure_token_cert_keyhash

    def get_css_project(self):

        if not self.use_code_sign_server:
            raise ValueError('Internal Error: get_css_project method should never get used if css is disabled')

        # The css_project maps to a specific css signing key
        if self.css_project is None:
            raise ValueError('Error: CSS Project not set')

        return self.css_project

    def keypair_file_from_sigchain_file(self, sigchain_file):
        assert(isinstance(sigchain_file, (str, unicode)))

        if self.use_code_sign_server:
            key_file_ext = '_pubkey.bin'
        else:
            key_file_ext = '.kp'

        keypair_file = sigchain_file
        if keypair_file.endswith(".qky"):
            keypair_file = keypair_file[:-4]

        return keypair_file + key_file_ext

    def get_keypair_file(self):
        if self.sign_keypair_file is not None:
            return self.sign_keypair_file
        else:
            if self.sign_keychain_file is None:
                return None
            else:
                return self.keypair_file_from_sigchain_file(self.sign_keychain_file)

    def codesign_exe_dir(self):

        if os.name == "nt":
            if "CSS_HOME" in os.environ:
                codesign_dir = os.path.join(os.environ['CSS_HOME'] + os.sep, "Intel", "LT CSS", "Bin")
            else:
                codesign_dir = os.path.join(os.environ['APPDATA'] + os.sep, "CSS_HOME", "Intel", "LT CSS", "Bin")

            if os.path.isdir(codesign_dir):
                return codesign_dir
            else:
                raise ValueError('The CSS Installation could not be found in the expected location: ' + codesign_dir)

        return None

    def codesign_exe(self):
        if self.codesign_exe_dir() is not None:
            codesign_exe = os.path.join(self.codesign_exe_dir(), "CodeSign.exe")
            if os.path.isfile(codesign_exe):
                return codesign_exe
        return "CodeSign"

    def ltsign_exe(self):
        if self.codesign_exe_dir() is not None:
            ltsign_exe = os.path.join(self.codesign_exe_dir(), "ltsign.exe")
            if os.path.isfile(ltsign_exe):
                return ltsign_exe
        return "ltsign"

    def generate_keypair_file(self, keypair_file=None):
        if keypair_file is None:
            keypair_file = self.get_keypair_file()
            assert(keypair_file is not None)

        if self.use_code_sign_server:
            raise ValueError("Cannot generate keypair file " + keypair_file + " because css is enabled. Generate this file using the CSS Tools and a USB Secure Token")

        keypair_file_dir = os.path.dirname(keypair_file)
        Mkdir().mkdir_p(keypair_file_dir)

        keytype = "ecdsa-secp384r1-sha384" # p384
        cmd = [ self.ltsign_exe(), "--createkey" ] \
            + [ "--algorithm", keytype ] \
            + [ "--output",    keypair_file ]
        self.execute(cmd, verbose=True)
        return keypair_file

    def ltsign(self, css_module_filename, css_signed_module_filename, css_keypair_filename):
        assert(not self.use_code_sign_server)
        # ltsign sign <css_module_file_in> <signed_css_module_file_out> <key_pair_file>
        cmd = [ self.ltsign_exe(), "--sign" ] \
            + [ "--input",       css_module_filename ] \
            + [ "--output",      css_signed_module_filename ] \
            + [ "--keypairfile", css_keypair_filename ]
        self.execute(cmd, verbose=self.verbose)

        # ltsign verify <css_module_file> <key_pair_file>
        cmd = [ self.ltsign_exe(), "--verify" ] \
            + [ "--input",       css_signed_module_filename ] \
            + [ "--keypairfile", css_keypair_filename ]
        self.execute(cmd, verbose=self.verbose)

    def passwd(self):
        self.css_secure_token_password = getpass.getpass('CSS USB Secure Token Password: ')
        self.codesign_testpass(self.css_secure_token_password)

    def codesign_debug_sign(self, css_module_filename, css_signed_module_filename):
        assert(self.use_code_sign_server)
        codesign_expected_output_filename = os.path.splitext(css_module_filename)[0] + "_signed" + os.path.splitext(css_module_filename)[1]
        try:
            # CodeSign.exe --wfsign --debug --project <css_project>  --input <css_module_file> --comment <comment> --password <secure_token_password>
            args = ["--wfsign", "--debug"]
            args += ["--comment", "Debug Sign" ]
            args += [ "--project", self.get_css_project(), "--input", css_module_filename ]

            cmd = [self.codesign_exe()] + args

            # let's not echo the token password, echo ******** instead
            cmd_echo = str.join(" ",cmd + ["--password", "********"])

            if self.css_secure_token_password is None:
                self.passwd()
            cmd += ["--password", self.css_secure_token_password]

            if self.css_secure_token_cert_keyhash is not None:
                # we assume keyhash matches format of viewrolesproject -- base64 string
                cmd += ["--keyhash", self.css_secure_token_cert_keyhash]
                cmd_echo += " --keyhash " + self.css_secure_token_cert_keyhash

            self.execute(cmd, verbose=self.verbose, cmd_echo=cmd_echo)

            if not FileUtils().filenames_are_equal(css_signed_module_filename, codesign_expected_output_filename):
                FileUtils.if_exists_delete_file(css_signed_module_filename)
                os.rename(codesign_expected_output_filename, css_signed_module_filename)

        finally:
            if not FileUtils().filenames_are_equal(css_signed_module_filename, codesign_expected_output_filename):
                FileUtils.if_exists_delete_file(codesign_expected_output_filename)

    def codesign_request(self, css_module_filename, sign_request_comment=None):
        # CodeSign.exe --wfsign --project PSGTest1 --input payload.bin --comment "comment"
        #              [--manager IDSID_LIST --security IDSID_LIST --reviewers IDSID_REVIEWERS]
        #   --> return code is sign request id
        assert(self.use_code_sign_server)

        # Note: if --debug is not specified, release is implied. This is a release command below.
        if self.get_css_project().startswith('PSGTest'):
            sign_request_args = ["--wfsign", "--quiet"]
        else:
            sign_request_args = ["--wfsign"]
        sign_request_args += [ "--project", self.get_css_project(), "--input", css_module_filename ]
        if sign_request_comment is None:
            sign_request_args += [ "--comment", "No Comment" ]
        else:
            sign_request_args += [ "--comment", sign_request_comment ]
        sign_request_cmd = [self.codesign_exe()] + sign_request_args
        sign_request_id, _ = self.execute(sign_request_cmd, verbose=self.verbose, expected_return_code=None)

        return sign_request_id

    def codesign_testpass(self, passwd):
        assert(self.use_code_sign_server)
    
        # CodeSign.exe --testpassword
        args = ["--testpassword"]

        cmd = [self.codesign_exe()] + args

        # let's not echo the token password, echo ******** instead
        cmd_echo = str.join(" ",cmd + ["--password", "********"])

        cmd += ["--password", passwd]

        try:
            Css.execute(self,cmd, verbose=1, cmd_echo=cmd_echo)

        except:
            raise ValueError('Incorrect USB Token password !!!')
    
    def codesign_cancel(self, css_module_filename, sign_request_id ):
        assert(self.use_code_sign_server)

        if self.get_css_project().startswith('PSGTest'):
            sign_cancel_args = ["--wfcancel", "--quiet"]
        else:
            sign_cancel_args = ["--wfcancel"]

        sign_cancel_args += ["--id", str(sign_request_id)]
        sign_cancel_cmd = [self.codesign_exe()] + sign_cancel_args
        sign_cancel_id, _ = self.execute(sign_cancel_cmd, verbose=self.verbose, expected_return_code=None)

        return sign_cancel_id

    def codesign_approve(self, sign_request_id, css_module_filename, approval_role):
        # CodeSign --wfapprove --id <request_id> --input payload.bin --password <****password****>
        #          --role <approval_role> [--keyhash <approval_key_hash>]
        assert(self.use_code_sign_server)

        valid_approval_roles = self.standard_approve_roles

        if self.debug:
            valid_approval_roles = self.debug_approve_roles

        assert(approval_role in valid_approval_roles)

        if self.css_secure_token_password is None:
            self.css_secure_token_password = getpass.getpass('CSS USB Secure Token Password: ')

        args = ["--wfapprove", "--id", str(sign_request_id)]
        args += ["--input", css_module_filename]

        cmd = [self.codesign_exe()] + args + ["--password", self.css_secure_token_password] + ["--role", approval_role]
        cmd_echo = str.join(" ",[self.codesign_exe()] + args + ["--password","********"] + ["--role",approval_role])

        self.execute(cmd, verbose=self.verbose, cmd_echo=cmd_echo)

    def parse_wfcomplete_output(self, output):
        return re.search('Output saved to (.*)$', output, re.MULTILINE).group(1).strip()

    def codesign_finalize(self, sign_request_id, css_module_filename, css_signed_module_filename):
        #  CodeSign --wfcomplete --id 1080 --input payload.bin --password <****password****>
        assert(self.use_code_sign_server)
        codesign_expected_output_filename = None

        if self.css_secure_token_password is None:
            self.css_secure_token_password = getpass.getpass('CSS USB Secure Token Password: ')

        try:
            args = ["--wfcomplete", "--id", str(sign_request_id)]
            args += ["--input", css_module_filename]
            cmd = [self.codesign_exe()] + args + ["--password",self.css_secure_token_password]
            cmd_echo = str.join(" ",[self.codesign_exe()] + args + ["--password","********"])
            _, out = self.execute(cmd, verbose=self.verbose, cmd_echo=cmd_echo)
            codesign_expected_output_filename = self.parse_wfcomplete_output(out)
            print "Expected File Name is: " + codesign_expected_output_filename

            if codesign_expected_output_filename is not None and not FileUtils().filenames_are_equal(css_signed_module_filename, codesign_expected_output_filename):
                FileUtils.if_exists_delete_file(css_signed_module_filename)
                os.rename(codesign_expected_output_filename, css_signed_module_filename)

        finally:
            if codesign_expected_output_filename is None or not FileUtils().filenames_are_equal(css_signed_module_filename, codesign_expected_output_filename):
                FileUtils.if_exists_delete_file(codesign_expected_output_filename)

    def create_tmp_file(self, desc="file", suffix=".tmp"):
        prefix = "tmp_css_" + desc + "_"
        if self.uid is not None:
            prefix = prefix + str(self.uid) + "_"
        return FileUtils.create_tmp_file(prefix=prefix, suffix=suffix, text=False, directory=self.work_dir)

    def css_module_to_tmp_file(self, css_module):
        assert(isinstance(css_module,CssModule))
        temp_module_file = self.create_tmp_file(desc=css_module.__class__.__name__.lower(), suffix=".bin")
        if self.debug: print " [DEBUG] Creating CSS Module File: " + temp_module_file
        css_module.save(filename=temp_module_file)
        return temp_module_file

    def sign_module_request(self, css_module):
        try:
            tmp_css_module_file = None
            throw_away_file = None

            tmp_css_module_file = self.css_module_to_tmp_file(css_module)
            if self.use_code_sign_server:
                sign_request_id = self.codesign_request(tmp_css_module_file)
            elif self.get_keypair_file() is not None and os.path.exists(self.get_keypair_file()):
                # test signing process (if you can) just for validation purposes and then throw away the result
                throw_away_file = self.create_tmp_file(suffix="_sig.bin")
                self.ltsign(tmp_css_module_file, throw_away_file, self.get_keypair_file())
                sign_request_id = 0
            else:
                sign_request_id = 0
        finally:
            for tmp_file in [tmp_css_module_file, throw_away_file]:
                FileUtils.if_exists_delete_file(tmp_file)

        return sign_request_id
	
    def sign_module_cancel(self, css_module, sign_request_id):
        try:
            tmp_css_module_file = None
            throw_away_file = None

            tmp_css_module_file = self.css_module_to_tmp_file(css_module)
            if self.use_code_sign_server:
                sign_request_id = self.codesign_cancel(css_module_filename=tmp_css_module_file, sign_request_id=sign_request_id)
            else:
                sign_request_id = 0
        finally:
            for tmp_file in [tmp_css_module_file, throw_away_file]:
                FileUtils.if_exists_delete_file(tmp_file)

        return sign_request_id	

    def sign_module_approve(self, css_module, sign_request_id, approval_role):
        try:
            tmp_css_module_file = None
            throw_away_file = None

            tmp_css_module_file = self.css_module_to_tmp_file(css_module)
            if self.use_code_sign_server:
                self.codesign_approve(sign_request_id=sign_request_id, css_module_filename=tmp_css_module_file, approval_role=approval_role)
            elif self.get_keypair_file() is not None and os.path.exists(self.get_keypair_file()):
                # test signing process (if you can) just for validation purposes and then throw away the result
                throw_away_file = self.create_tmp_file(suffix="_sig.bin")
                self.ltsign(tmp_css_module_file, throw_away_file, self.get_keypair_file())
        finally:
            for tmp_file in [tmp_css_module_file, throw_away_file]:
                FileUtils.if_exists_delete_file(tmp_file)

    def sign_module_finalize(self, css_module, sign_request_id, enable_multi=False):
        try:
            tmp_css_module_file = None
            tmp_css_module_signed_file = None

            tmp_css_module_file = self.css_module_to_tmp_file(css_module)
            tmp_css_module_signed_file = self.create_tmp_file(suffix="_sig.bin")

            if self.use_code_sign_server:
                self.codesign_finalize(sign_request_id=sign_request_id,
                                       css_module_filename=tmp_css_module_file,
                                       css_signed_module_filename=tmp_css_module_signed_file)
            else:
                assert(self.get_keypair_file() is not None and os.path.exists(self.get_keypair_file()))
                self.ltsign(tmp_css_module_file, tmp_css_module_signed_file, self.get_keypair_file())

            if isinstance(css_module,PublicKeyCssModule):
                css_module_signed = PublicKeyCssModule(enable_multi=enable_multi)
            else:
                module_type = type(css_module)
                css_module_signed = module_type()
            css_module_signed.load(filename=tmp_css_module_signed_file)

        finally:
            for tmp_file in [tmp_css_module_file]:
                FileUtils.if_exists_delete_file(tmp_file)

        return css_module_signed

    def sign_module(self, css_module):

        assert(isinstance(css_module,CssModule))
        if not self.use_code_sign_server:
            assert(os.path.exists(self.get_keypair_file()))

        try:
            temp_module_file = None
            temp_module_signed_file = None

            temp_module_file = self.css_module_to_tmp_file(css_module)

            temp_module_signed_file = self.create_tmp_file(suffix="_sig.bin")

            if self.use_code_sign_server:
                if isinstance(css_module,PublicKeyCssModule):
                    assert(self.debug)
                    req_id = self.codesign_request(temp_module_file)
                    assert(req_id != None)
                    for approval_role in self.debug_approve_roles:
                        self.codesign_approve(req_id, temp_module_file, approval_role)
                    self.codesign_finalize(req_id, temp_module_file, temp_module_signed_file)
                else:
                    self.codesign_debug_sign(temp_module_file, temp_module_signed_file)
            else:
                self.ltsign(temp_module_file, temp_module_signed_file, self.get_keypair_file())

            module_type = type(css_module)
            css_module_signed = module_type()

            css_module_signed.load(filename=temp_module_signed_file)
        finally:
            for tmp_file in [temp_module_file, temp_module_signed_file]:
                FileUtils.if_exists_delete_file(tmp_file)

        return css_module_signed

    def sign_cert(self, cert):
        assert(self.sign_keychain_file is not None)
        assert(isinstance(cert, EngineeringCert))

        # make a deep copy of cert object so that input cert is not modified
        cert = copy.deepcopy(cert)

        module = self.extract_module_from_cert(cert)
        signed_module = self.sign_module(module)
        assert(signed_module.engineering_cert_header().get_value(offset=0,size=4096) == cert.engineering_cert_header().get_value(offset=0,size=4096))
        assert(module.signature().get_value(offset=0,size=112) != signed_module.signature().get_value(offset=0,size=112))

        block0_sig = SignatureChainBlock0Entry().initialize()
        block0_sig.set_signature(signed_module.signature())
        block0_sig.validate()

        sigchain = SignatureChain().load(filename=self.sign_keychain_file)
        sigchain.append(block0_sig)
        sigchain.validate()

        sigchain_appended = False
        for i in range(0,4):
            if cert.signature_descriptor().signatures().signature_chain(i).size() == 0:
                cert.signature_descriptor().signatures().signature_chain(i).append(sigchain)
                assert(cert.signature_descriptor().signatures().signature_chain(i).size() == sigchain.size())
                sigchain_appended = True
                break
        assert(sigchain_appended)
        cert.update()
        cert.validate()
        return cert

    # This function is here to support ephemeral signing (one signing server call per zip file)
        #    see case 591232
    def sign_eph(self, filestosign): # filestosign=dictionary of filenames and data
        assert(self.sign_keychain_file is not None)
        returndata = {}
        cmf_sign = CmfSign()
        try:
            cmf_sign.create_tmp_files(filestosign)
            cmf_sign.sign_files()

            # Load the Root (and Limited signing key)
            sigchain = SignatureChain().load(filename=self.sign_keychain_file)

            # Now to sign the newly created pkey
            newkeyfilename = cmf_sign.get_pub_key_filename()
            temp_module_signed_file = newkeyfilename[0:-7] + "_signed.module"
            if self.use_code_sign_server:
                self.codesign_debug_sign(newkeyfilename, temp_module_signed_file)
            else:
                self.ltsign(newkeyfilename, temp_module_signed_file, self.get_keypair_file())
            key_module = SignatureChainPublicKeyEntry().load(temp_module_signed_file)

            # Append the block0 signatures to each module and store for returning
            for filename in filestosign:
                tempchain = SignatureChain()
                tempchain.append(sigchain.signature_entry(0))
                if tempchain.number_of_signature_entries() > 1:
                    tempchain.append(sigchain.signature_entry(1))
                tempchain.append(key_module)

                block0sig = cmf_sign.get_signature(filename)
                tempchain.append(block0sig)
                returndata[filename] = tempchain
        finally:
            cmf_sign.cleanup()
        return returndata

    def sign_cmf(self, cmf):

        assert(self.sign_keychain_file is not None)
        assert(isinstance(cmf, Cmf) or isinstance(cmf, BitstreamZip) or isinstance(cmf, BitstreamFragmentCollection))

        # make a deep copy of cmf object so that input cmf is not modified
        cmf = copy.deepcopy(cmf)

        if isinstance(cmf, BitstreamZip):
            for f in cmf.fragments:
                if Css.needs_signed(cmf.fragments[f]):
                    cmf.fragments[f] = self.sign_cmf(cmf=cmf.fragments[f])
        elif isinstance(cmf, BitstreamFragmentCollection) and not isinstance(cmf, Cmf):
            for i in range(cmf.number_of_fragments()):
                if Css.needs_signed(cmf.fragments[i]):
                    cmf.fragments[i] = self.sign_cmf(cmf=cmf.fragments[i])
        else:
            assert(isinstance(cmf, Cmf))
            enable_multi = cmf.cmf_descriptor().is_fm_format()
            module = CmfDescriptorCssModule(cmf_descriptor=cmf.cmf_descriptor(), signature=Signature(enable_multi=enable_multi).initialize())
            signed_module = self.sign_module(module)
            assert(signed_module.cmf_descriptor().get_value(offset=0,size=4096) == cmf.cmf_descriptor().get_value(offset=0,size=4096))
            assert(module.signature().get_value(offset=0,size=112) != signed_module.signature().get_value(offset=0,size=112))

            block0_sig = SignatureChainBlock0Entry(enable_multi=enable_multi).initialize()
            block0_sig.set_signature(signed_module.signature())
            block0_sig.validate()

            sigchain = SignatureChain().load(filename=self.sign_keychain_file)
            sigchain.append(block0_sig)
            sigchain.validate()

            # This check is only needed within intel. External customer who signs CMF can use the last two slots
            # Also, the debug flow is allowed to sign extra times so only check if we're using the code sign server.
            if self.use_code_sign_server:
                if cmf.signature_descriptor().signatures().signature_chain(0).size() != 0 and cmf.signature_descriptor().signatures().signature_chain(1).size() != 0:
                    raise ValueError('Cannot sign this cmf file because it appears as if it has already been signed twice')

            sigchain_appended = False
            for i in range(0,cmf.signature_descriptor().signatures().number_of_signature_chains()):
                if cmf.signature_descriptor().signatures().signature_chain(i).size() == 0:
                    cmf.signature_descriptor().signatures().signature_chain(i).append(sigchain)
                    assert(cmf.signature_descriptor().signatures().signature_chain(i).size() == sigchain.size())
                    sigchain_appended = True
                    break
            assert(sigchain_appended)
            cmf.update()

        cmf.validate()

        return cmf

    # Initiate sign request(s) to a cmf file or collections of cmf files (e.g. nadder.zip)
    # If id_map_file is provided, then write the mapping of module hashes and request ids to that file
    def sign_cmf_request(self, cmf, id_map_file=None):

        assert(self.sign_keychain_file is not None)
        assert(isinstance(cmf, Cmf) or isinstance(cmf, BitstreamZip) or isinstance(cmf, BitstreamFragmentCollection))

		#Check if QKY file exists
        if not os.path.exists(self.sign_keychain_file):
            raise ValueError("Keychain file does not exist: " + self.sign_keychain_file)
			
        # Check file existence
        if id_map_file is not None:
            if not os.access(id_map_file, os.R_OK) or not os.access(id_map_file, os.W_OK):
                raise ValueError("No read/write permission to access the given mapping file: " + id_map_file)

        # make a deep copy of cmf object so that input cmf is not modified
        cmf = copy.deepcopy(cmf)

        if isinstance(cmf, BitstreamZip):
            for f in cmf.fragments:
                if isinstance(cmf.fragments[f], Cmf) and cmf.fragments[f].cmf_descriptor().descriptor_format() == "SDM-full":
                    self.sign_cmf_request(cmf=cmf.fragments[f], id_map_file=id_map_file)
        elif isinstance(cmf, BitstreamFragmentCollection) and not isinstance(cmf, Cmf):
            for i in range(cmf.number_of_fragments()):
                if isinstance(cmf.fragments[i], Cmf) and cmf.fragments[i].cmf_descriptor().descriptor_format() == "SDM-full":
                    self.sign_cmf_request(cmf=cmf.fragments[i], id_map_file=id_map_file)
        else:
            assert(isinstance(cmf, Cmf))
            module = CmfDescriptorCssModule(cmf_descriptor=cmf.cmf_descriptor(), signature=Signature().initialize())
            sign_request_id = self.sign_module_request(module)

            # Save the mapping module SHA384 hash and sign request ID to the file
            if id_map_file is not None:
                map_str = cmf.filename + ":" + str(sign_request_id) + "\n"
                with open(id_map_file, 'a') as id_map_file_object:
                    id_map_file_object.write(map_str)

            return sign_request_id
			
    # Initiate sign cancel(s) to a cmf file or collections of cmf files (e.g. nadder.zip)
    # If id_map_file is provided, then read the mappings of module hash and request_id(s) from that file
    def sign_cmf_cancel(self, cmf, sign_request_id=None, id_map_file=None):

        assert(isinstance(cmf, Cmf) or isinstance(cmf, BitstreamZip) or isinstance(cmf, BitstreamFragmentCollection))
        if sign_request_id is None and id_map_file is None:
            raise ValueError("No request ID was given. Cannot approve sign request. ")

        if sign_request_id is not None and not isinstance(cmf, Cmf):
            raise ValueError("Cannot approve multiple CMF files with one request ID. ")

        # Check file existence
        if id_map_file is not None:
            if not os.access(id_map_file, os.R_OK):
                raise ValueError("No read permission to access the given mapping file: " + id_map_file)

        assert(not (sign_request_id is not None and id_map_file is not None))
		
        # make a deep copy of cmf object so that input cmf is not modified
        cmf = copy.deepcopy(cmf)

        if isinstance(cmf, BitstreamZip):
            for f in cmf.fragments:
                if isinstance(cmf.fragments[f], Cmf) and cmf.fragments[f].cmf_descriptor().descriptor_format() == "SDM-full":
                    self.sign_cmf_cancel(cmf=cmf.fragments[f], sign_request_id=sign_request_id, id_map_file=id_map_file)
        elif isinstance(cmf, BitstreamFragmentCollection) and not isinstance(cmf, Cmf):
            for i in range(cmf.number_of_fragments()):
                if isinstance(cmf.fragments[i], Cmf) and cmf.fragments[i].cmf_descriptor().descriptor_format() == "SDM-full":
                    self.sign_cmf_cancel(cmf=cmf.fragments[i], sign_request_id=sign_request_id, id_map_file=id_map_file)
        else:
            assert(isinstance(cmf, Cmf))
            module = CmfDescriptorCssModule(cmf_descriptor=cmf.cmf_descriptor(), signature=Signature().initialize())
            

            # Find the request ID from mapping file
            if id_map_file is not None:
                with open(id_map_file) as id_map_file_object:
                    for line in id_map_file_object.readlines():
                        if cmf.filename in line:
                            sign_request_id = line.split(':')[1].strip()
                            break
                if sign_request_id is None:
                    raise ValueError("Cannot find cmf filename " + cmf.filename + " in the mapping file: " + id_map_file)

            return self.sign_module_cancel(module, sign_request_id)

    # Approve sign request(s) to a cmf file or collections of cmf files (e.g. nadder.zip)
    # If id_map_file is provided, then read the mappings of module hash and request_id(s) from that file
    def sign_cmf_approve(self, cmf, approval_role, sign_request_id=None, id_map_file=None):

        assert(self.sign_keychain_file is not None)
        assert(isinstance(cmf, Cmf) or isinstance(cmf, BitstreamZip) or isinstance(cmf, BitstreamFragmentCollection))

        if sign_request_id is None and id_map_file is None:
            raise ValueError("No request ID was given. Cannot approve sign request. ")

        if sign_request_id is not None and not isinstance(cmf, Cmf):
            raise ValueError("Cannot approve multiple CMF files with one request ID. ")

        # Check file existence
        if id_map_file is not None:
            if not os.access(id_map_file, os.R_OK):
                raise ValueError("No read permission to access the given mapping file: " + id_map_file)

        assert(not (sign_request_id is not None and id_map_file is not None))

        # make a deep copy of cmf object so that input cmf is not modified
        cmf = copy.deepcopy(cmf)

        if isinstance(cmf, BitstreamZip):
            for f in cmf.fragments:
                if isinstance(cmf.fragments[f], Cmf) and cmf.fragments[f].cmf_descriptor().descriptor_format() == "SDM-full":
                    self.sign_cmf_approve(cmf=cmf.fragments[f], approval_role=approval_role, \
                                          sign_request_id=sign_request_id, id_map_file=id_map_file)
        elif isinstance(cmf, BitstreamFragmentCollection) and not isinstance(cmf, Cmf):
            for i in range(cmf.number_of_fragments()):
                if isinstance(cmf.fragments[i], Cmf) and cmf.fragments[i].cmf_descriptor().descriptor_format() == "SDM-full":
                    self.sign_cmf_approve(cmf=cmf.fragments[i], approval_role=approval_role, \
                                          sign_request_id=sign_request_id, id_map_file=id_map_file)
        else:
            assert(isinstance(cmf, Cmf))
            module = CmfDescriptorCssModule(cmf_descriptor=cmf.cmf_descriptor(), signature=Signature().initialize())

            # Find the request ID from mapping file
            if id_map_file is not None:
                with open(id_map_file) as id_map_file_object:
                    for line in id_map_file_object.readlines():
                        if cmf.filename in line:
                            sign_request_id = line.split(':')[1].strip()
                            break
                if sign_request_id is None:
                    raise ValueError("Cannot find cmf filename " + cmf.filename + " in the mapping file: " + id_map_file)

            self.sign_module_approve(module, sign_request_id, approval_role)

    # Finalize sign request(s) to a cmf file or collections of cmf files (e.g. nadder.zip)
    # If id_map_file is provided, then read the mappings of module hash and request_id(s) from that file
    def sign_cmf_finalize(self, cmf, sign_request_id=None, id_map_file=None):

        assert(self.sign_keychain_file is not None)
        assert(isinstance(cmf, Cmf) or isinstance(cmf, BitstreamZip) or isinstance(cmf, BitstreamFragmentCollection))

		#Check if QKY file exists
        if not os.path.exists(self.sign_keychain_file):
            raise ValueError("Keychain file does not exist: " + self.sign_keychain_file)
			
        if sign_request_id is None and id_map_file is None:
            raise ValueError("No request ID was given. Cannot approve sign request. ")

        if sign_request_id is not None and not isinstance(cmf, Cmf):
            raise ValueError("Cannot approve multiple CMF files with one request ID. ")

        # Check file existence
        if id_map_file is not None:
            if not os.access(id_map_file, os.R_OK):
                raise ValueError("No read permission to access the given mapping file: " + id_map_file)

        assert(not (sign_request_id is not None and id_map_file is not None))

        # make a deep copy of cmf object so that input cmf is not modified
        cmf = copy.deepcopy(cmf)

        if isinstance(cmf, BitstreamZip):
            for f in cmf.fragments:
                if isinstance(cmf.fragments[f], Cmf) and cmf.fragments[f].cmf_descriptor().descriptor_format() == "SDM-full":
                    cmf.fragments[f] = self.sign_cmf_finalize(cmf=cmf.fragments[f], sign_request_id=sign_request_id, id_map_file=id_map_file)
        elif isinstance(cmf, BitstreamFragmentCollection) and not isinstance(cmf, Cmf):
            for i in range(cmf.number_of_fragments()):
                if isinstance(cmf.fragments[i], Cmf) and cmf.fragments[i].cmf_descriptor().descriptor_format() == "SDM-full":
                    cmf.fragments[i] = self.sign_cmf_finalize(cmf=cmf.fragments[i], sign_request_id=sign_request_id, id_map_file=id_map_file)
        else:
            assert(isinstance(cmf, Cmf))
            enable_multi = cmf.cmf_descriptor().is_fm_format()
            module = CmfDescriptorCssModule(cmf_descriptor=cmf.cmf_descriptor(), signature=Signature(enable_multi=enable_multi).initialize())

            # Find the request ID from mapping file
            if id_map_file is not None:
                with open(id_map_file) as id_map_file_object:
                    for line in id_map_file_object.readlines():
                        if cmf.filename in line:
                            sign_request_id = line.split(':')[1].strip()
                            break
                if sign_request_id is None:
                    raise ValueError("Cannot find cmf filename " + cmf.filename + " in the mapping file: " + id_map_file)

            signed_module = self.sign_module_finalize(module, sign_request_id)
            assert(signed_module.cmf_descriptor().get_value(offset=0,size=4096) == cmf.cmf_descriptor().get_value(offset=0,size=4096))
            assert(module.signature().get_value(offset=0,size=112) != signed_module.signature().get_value(offset=0,size=112))

            block0_sig = SignatureChainBlock0Entry(enable_multi=enable_multi).initialize()
            block0_sig.set_signature(signed_module.signature())
            block0_sig.validate()

            sigchain = SignatureChain().load(filename=self.sign_keychain_file)
            sigchain.append(block0_sig)
            sigchain.validate()

            # This check is only needed within intel. External customer who signs CMF can use the last two slots
            # Also, the debug flow is allowed to sign extra times so only check if we're using the code sign server.
            if self.use_code_sign_server:
                if cmf.signature_descriptor().signatures().signature_chain(0).size() != 0 and cmf.signature_descriptor().signatures().signature_chain(1).size() != 0:
                    raise ValueError('Cannot sign this cmf file because it appears as if it has already been signed twice')

            sigchain_appended = False
            for i in range(0,cmf.signature_descriptor().signatures().number_of_signature_chains()):
                if cmf.signature_descriptor().signatures().signature_chain(i).size() == 0:
                    cmf.signature_descriptor().signatures().signature_chain(i).append(sigchain)
                    assert(cmf.signature_descriptor().signatures().signature_chain(i).size() == sigchain.size())
                    sigchain_appended = True
                    break
            assert(sigchain_appended)
            cmf.update()

        cmf.validate()

        return cmf

    def create_root_key(self, keypair_file, hash_sel=None):
        if not os.path.exists(keypair_file):
            self.generate_keypair_file(keypair_file=keypair_file)
        root_key = EcdsaPublicKey().initialize(sexp_file=keypair_file, binary=True)
        # FB Case:458059 - Root Key Cancellation value is 0xffffffff
        root_key.set_cancel_id(cancel_id=0xffffffff)
        sigchain_root_key = SignatureChainRootEntry().initialize(codesign_public_key=root_key, hash_sel=hash_sel)
        sigchain_root_key.validate()
        return sigchain_root_key

    def create_multi_root_key(self, keypair_file0, keypair_file1=None, keypair_file2=None, hash_sel=None):
        if not os.path.exists(keypair_file0):
            self.generate_keypair_file(keypair_file=keypair_file0)
        root_key0 = EcdsaPublicKey(enable_multi=True).initialize(sexp_file=keypair_file0, binary=True)
        # FB Case:458059 - Root Key Cancellation value is 0xffffffff
        root_key0.set_cancel_id(cancel_id=0xffffffff)

        root_key1=None
        if keypair_file1 is not None:
            if not os.path.exists(keypair_file1):
                self.generate_keypair_file(keypair_file=keypair_file1)
            root_key1 = EcdsaPublicKey(enable_multi=True).initialize(sexp_file=keypair_file1, binary=True)
            root_key1.set_cancel_id(cancel_id=0xffffffff)

        root_key2=None
        if keypair_file2 is not None:
            if not os.path.exists(keypair_file2):
                self.generate_keypair_file(keypair_file=keypair_file2)
            root_key2 = EcdsaPublicKey(enable_multi=True).initialize(sexp_file=keypair_file2, binary=True)
            root_key2.set_cancel_id(cancel_id=0xffffffff)

        sigchain_multi_root_key = SignatureChainMultiRootEntry().initialize(hash_sel=hash_sel, codesign_public_key0=root_key0, codesign_public_key1=root_key1, codesign_public_key2=root_key2)
        sigchain_multi_root_key.validate()
        return sigchain_multi_root_key

    def sign_key(self, keypair_file):
        assert(self.sign_keychain_file is not None)
        #assert(not self.use_code_sign_server)

        if not os.path.exists(keypair_file):
            self.generate_keypair_file(keypair_file=keypair_file)

        key = EcdsaPublicKey().initialize(sexp_file=keypair_file, binary=True)
        key.set_permission(self.permission)
        key.set_cancel_id(self.cancel_id)
        module = PublicKeyCssModule().initialize(codesign_public_key=key)

        signed_module = self.sign_module(module)

        assert(signed_module.get_value(offset=0,size=0x90) == module.get_value(offset=0,size=0x90))
        assert(module.signature().get_value(offset=0,size=112) != signed_module.signature().get_value(offset=0,size=112))

        sigchain = SignatureChain().load(filename=self.sign_keychain_file)
        sigchain.append(signed_module.public_key_entry())
        sigchain.validate()

        return sigchain

    def create_pubkey_module(self, keypair_file, enable_multi=False):
        if not self.use_code_sign_server and not os.path.exists(keypair_file):
            self.generate_keypair_file(keypair_file=keypair_file)

        if not os.path.exists(keypair_file):
            raise ValueError("Keypair file does not exist: " + keypair_file)

        key = EcdsaPublicKey(enable_multi=enable_multi, in_sigchain_pubkey_entry=True).initialize(sexp_file=keypair_file, binary=True)
        key.set_permission(self.permission)
        key.set_cancel_id(self.cancel_id)
        module = PublicKeyCssModule(enable_multi=enable_multi).initialize(codesign_public_key=key)

        return module

    def complete_sigchain_with_approved_module(self, sign_request_id, module, signed_module_file, enable_multi=False):
        assert(self.sign_keychain_file is not None)
        assert(isinstance(module, PublicKeyCssModule))

        # Finalize signing of approved module
        signed_module = self.sign_module_finalize(css_module=module, sign_request_id=sign_request_id, enable_multi=enable_multi)
        signed_module.save(signed_module_file)

        # Create Signature Chain with sign_keychain_file (e.g. *root*.qky)
        sigchain = SignatureChain().load(filename=self.sign_keychain_file)

        # Append the signed public key entry from *.module to the signature chain
        sigchain.append(signed_module.public_key_entry())
        sigchain.validate()

        assert(sigchain.last_entry().signature().get_value(offset=0,size=112) != 0)

        return sigchain

    def extract_module_from_qky(self, qky_signature_chain, enable_multi=False):
        key = qky_signature_chain.last_key()
        module = PublicKeyCssModule(enable_multi=enable_multi).initialize(codesign_public_key=key)
        return module

    def extract_module_from_cert(self, cert):
        module = EngineeringCertHeaderCssModule(engineering_cert_header=cert.engineering_cert_header(),
                                                signature=Signature().initialize())
        return module

    def sign_key_request(self, keypair_file, skip_css_request=False):
        assert(self.sign_keychain_file is not None)

        if not self.use_code_sign_server and not os.path.exists(keypair_file):
            self.generate_keypair_file(keypair_file=keypair_file)

        if not os.path.exists(keypair_file):
            raise ValueError("Keypair file does not exist: " + keypair_file)

        key = EcdsaPublicKey().initialize(sexp_file=keypair_file, binary=True)
        key.set_permission(self.permission)
        key.set_cancel_id(self.cancel_id)
        module = PublicKeyCssModule().initialize(codesign_public_key=key)

        if skip_css_request:
            request_id = 0
            print "Skipping CSS Sign Request..."
        else:
            request_id = self.sign_module_request(css_module=module)

        sigchain = SignatureChain().load(filename=self.sign_keychain_file)
        sigchain.append(module.public_key_entry())
        sigchain.validate()

        return (request_id, sigchain)
		
    def sign_key_approve(self, sign_request_id, qky_signature_chain, approval_role):
        assert(isinstance(qky_signature_chain, SignatureChain))
        module = self.extract_module_from_qky(qky_signature_chain)
        self.sign_module_approve(css_module=module, sign_request_id=sign_request_id, approval_role=approval_role)

    def sign_key_finalize(self, sign_request_id, qky_signature_chain):
        assert(isinstance(qky_signature_chain, SignatureChain))

        module = self.extract_module_from_qky(qky_signature_chain)
        signed_module = self.sign_module_finalize(css_module=module, sign_request_id=sign_request_id)

        signed_qky = copy.deepcopy(qky_signature_chain)
        signed_qky.validate()
        assert(signed_qky.last_entry() is not None)

        signed_qky.last_entry().set_signature(signature=signed_module.signature())
        assert(signed_qky.last_entry().signature().get_value(offset=0,size=112) != 0)
        assert(signed_qky.get_value(offset=0,size=signed_qky.size()-112) == qky_signature_chain.get_value(offset=0,size=qky_signature_chain.size()-112))
        assert(signed_qky.get_value(offset=signed_qky.size()-112,size=112) != qky_signature_chain.get_value(offset=qky_signature_chain.size()-112,size=112))

        return signed_qky

    def sign_cert_request(self, cert, skip_css_request=False):
        assert(self.sign_keychain_file is not None)
        assert(cert.signature_descriptor().signatures().number_of_signature_chains() != 0)
        for i in range(0, cert.signature_descriptor().signatures().number_of_signature_chains()):
          assert(cert.signature_descriptor().signatures().signature_chain(i).number_of_signature_entries() == 0)

        sigchain = SignatureChain().load(filename=self.sign_keychain_file)
        module = self.extract_module_from_cert(cert)

        if skip_css_request:
            request_id = 0
            print "Skipping CSS Sign Request..."
        else:
            request_id = self.sign_module_request(css_module=module)


        cert_css = copy.deepcopy(cert)
        for  i in range(0, sigchain.number_of_signature_entries()):
          cert_css.signature_descriptor().signatures().signature_chain(0).append( sigchain.signature_entry(i) )

        cert_css.update()
        cert_css.validate()

        return (request_id, cert_css)

    def sign_cert_approve(self, sign_request_id, cert, approval_role):
        assert(isinstance(cert, EngineeringCert))

        module = self.extract_module_from_cert(cert)
        self.sign_module_approve(css_module=module, sign_request_id=sign_request_id, approval_role=approval_role)

    def sign_cert_finalize(self, sign_request_id, cert):
        assert(isinstance(cert, EngineeringCert))

        module = self.extract_module_from_cert(cert)
        signed_module = self.sign_module_finalize(css_module=module, sign_request_id=sign_request_id)

        block0_sig = SignatureChainBlock0Entry().initialize()
        block0_sig.set_signature(signed_module.signature())
        block0_sig.validate()

        signed_cert = copy.deepcopy(cert)
        if (signed_cert.signature_descriptor().signatures().signature_chain(1).number_of_signature_entries() != 0):
          print "WARNING: Expected there to be only 1 signature chain during finalize. Since module finalize is already complete, writing result anyway"
        signed_cert.signature_descriptor().signatures().signature_chain(0).last_entry().set_signature(signature=signed_module.signature())
        signed_cert.update()
        signed_cert.validate()

        return signed_cert

    @staticmethod
    def needs_signed(fragment):
        if isinstance(fragment, Cmf):
            return fragment.cmf_descriptor().descriptor_format() in ["SDM-full", "FM"]
        return False

class CssTest(unittest.TestCase):

    def test_constructor(self):
        css = Css()
        self.assertNotEqual(css, None)

    def test_sign_cmf(self):
        cmf = Cmf()
        cmf.load("../test/files/example.cmf")
        css = Css()
        css.sign_keypair_file = "../test/keys/public0_p384.kp"
        css.sign_keychain_file = "../test/keys/codesign1.qky"
        css.sign_cmf(cmf)

    def test_sign_cert(self):
        cert = EngineeringCert()
        cert.load("../test/files/example_eng_cert.cert")
        css = Css()
        css.sign_keypair_file = "../test/keys/public0_p384.kp"
        css.sign_keychain_file = "../test/keys/codesign1.qky"
        css.sign_cert(cert)

    def test_generate_keypair_file(self):
        css = Css()
        css.generate_keypair_file("../work_dir/generate_kp_file.kp")
        css.sign_keychain_file = "../work_dir/generate_kp_file.qky"
        css.generate_keypair_file()
        css.sign_keypair_file = "../work_dir/generate_kp_file.kp"
        css.generate_keypair_file()

    def test_create_keychain(self):
        css = Css()
        css.sign_keychain_file = "../work_dir/root_a.qky"
        root_key = css.create_root_key(keypair_file=css.generate_keypair_file())
        root_key.save(filename=css.sign_keychain_file)

        new_keychain = css.sign_key(keypair_file="../test/keys/public0_p384.kp")
        new_keychain.save("../work_dir/codesign_a.qky")

    def test_key_mkdir(self):
        css = Css()
        css.sign_keychain_file = "../work_dir/subdir1/foo/bar/keys/root.qky"
        css.create_root_key(keypair_file=css.generate_keypair_file())

    def test_css_dash_help(self):
        css = Css()

        print css.codesign_exe()

        if os.name == "nt":
            self.assertNotEquals(css.codesign_exe(), "CodeSign")
            self.assertNotEquals(css.ltsign_exe(), "ltsign")
        else:
            self.assertEquals(css.codesign_exe(), "CodeSign")
            self.assertEquals(css.ltsign_exe(), "ltsign")

        css.execute([css.codesign_exe(), "--help"], verbose=True)
        css.execute([css.ltsign_exe(), "--help"], verbose=True)

    def test_ltcss_version(self):
        """
        Verify that LTCSS version is 4.5.2.0 or above. This is needed because --testpassword feature was included
        starting with 4.5.2.0.
        """

        css = Css()

        #
        # CodeSign will print the version on the 2nd line:
        # ie: Intel LT-CSS command line utility, v4.4.0.0
        # This is true for {windows,linux} x {4.4, 4.5}
        #

        (_, out) = css.execute([css.codesign_exe(), "--help"], verbose=False)
        out = out.splitlines()[1]

        # Extracts vA.B.C.D
        match = re.search(r'\b(v\d+(\.\d+)*\b)', out)

        # Strip leading 'v'
        out = match.group()[1:]

        # Turn the A.B.C.D into a tuple of int's and compare
        version = tuple(map(int, out.split('.')))

        minVersion = ( 4, 5, 2, 0 )
        self.assertTrue(version >= minVersion)

    def test_ltsign_req_approve_finalize(self):

        css = Css()
        css.sign_keychain_file = "../work_dir/root_b.qky"
        root_key = css.create_root_key(keypair_file=css.generate_keypair_file())
        root_key.save(filename=css.sign_keychain_file)

        req_id, qky = css.sign_key_request(keypair_file="../work_dir/codesign_b.kp")
        self.assertEqual(req_id, 0)
        self.assertTrue(qky is not None)

        for role in ["reviewer", "manager", "security"]:
            css.sign_key_approve(sign_request_id=req_id, qky_signature_chain=qky, approval_role=role)

        signed_qky = css.sign_key_finalize(sign_request_id=req_id, qky_signature_chain=qky)
        signed_qky.validate()

        #print signed_qky

    def test_parse_wfcomplete(self):
        css = Css()
        output="Sign command succeeded.\nOutput saved to c:\\users\\acopelan\\appdata\\local\\temp\\tmp_css_publickeycssmodule_vzhpqo.1521651524.83919.0_sig.bin\n\n"
        expected = 'c:\\users\\acopelan\\appdata\\local\\temp\\tmp_css_publickeycssmodule_vzhpqo.1521651524.83919.0_sig.bin'
        self.assertEqual(css.parse_wfcomplete_output(output), expected)
        print "Parsed " + expected + " out of output:\n" + output

