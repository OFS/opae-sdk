##########################
#
# All about signing
#
# As long as we are still using the same crypto IP/FW [and openssl], this file should not change
#
# If customer does not want to use openssl (prefer HSM instead), I expect there might be minor change in this module
#   If key_manager can be replaced with something that interface with HSM but still provide equivalent API, then 
#   required changes should be very minimum
#
##########################
import common_util
import openssl
import database
import key_manager
import bitstream
import keychain
        
class SIGNER(object) :

    def __init__(self, family) :
    
        self.family = family.upper()
        common_util.assert_in_error(family in database.FAMILY_LIST, "Supported family is %s, but found %s" % (database.FAMILY_LIST.keys(), family))
        self.openssl = openssl.openssl()
        self.key_manager = key_manager.KEY_MANAGER(self.openssl)
        self.database = database.FAMILY_LIST[self.family]
        
    def __del__(self) :
    
        self.clean()
            
    def __exit__(self) :
    
        self.clean()
        
    def clean(self) :
    
        if self.key_manager != None :
            del self.key_manager
            self.key_manager = None
            
        if self.openssl != None :
            del self.openssl
            self.openssl = None
            
    def make_private_pem(self, ecparam, private_pem, encrypt) :

        self.key_manager.make_private_pem(ecparam, private_pem, encrypt)
        return True

    def make_public_pem(self, private_pem, public_pem) :

        self.key_manager.make_public_pem(private_pem, public_pem)
        return True
        
    def make_root(self, public_pem, output_qky) :
    
        data = common_util.BYTE_ARRAY()
        public_key = self.key_manager.get_public_key(public_pem)
        common_util.assert_in_error(self.family in public_key.curve_info.supported_families, "Family %s does not support the specified PEM %s curve type %s" % (self.family, public_pem, public_key.curve_info.name))
        self.create_root_entry(data, public_key)
        del public_key
        
        # Write to file
        file = open(output_qky, "wb")
        data.data.tofile(file)
        file.close()
        return True

    def append_key(self, input_qky, private_pem, public_pem, output_qky, permission, cancel) :
    
        qky = keychain.QKY_READER(input_qky, self.openssl, self.family, None, None)
        common_util.assert_in_error(qky.complete == False, "Input %s should not have a complete signing chain" % input_qky)

        if self.database.SUPPORTED_TYPES == None :
            common_util.assert_in_error(permission == None, "Family %s does not support bitstream type, but found 0x%08X", self.family, permission)
            permission = -1
        else :
            common_util.assert_in_error(permission != None, "Permission must be specified")
            common_util.assert_in_error((qky.permission & permission) != 0, "The specified permission results chain permission value of 0. This is invalid to sign any data")
        if self.database.SUPPORTED_CANCELS == None :
            common_util.assert_in_error(cancel == None, "Family %s does not support cancellation, but found 0x%08X", self.family, cancel)
            cancel = -1
        else :
            common_util.assert_in_error(cancel != None, "Cancel ID must be specified")
            common_util.assert_in_error(cancel in self.database.SUPPORTED_CANCELS, "Cancel ID of 0x%08X is not supported by family %s" % (cancel, self.family))

        private_key = self.key_manager.get_private_key(private_pem)
        common_util.assert_in_error(self.family in private_key.curve_info.supported_families, "Family %s does not support the specified PEM %s curve type %s" % (self.family, private_pem, private_key.curve_info.name))
        common_util.assert_in_error(private_key.curve_info.enum == qky.keys[0].curve_info.enum, "QKY input %s (%s) does not match with key from PEM file %s (%s)" % (input_qky, qky.keys[0].curve_info.name, private_pem, private_key.curve_info.name))
        private_key.match_xy(qky.keys[-1].key)
        
        public_key = self.key_manager.get_public_key(public_pem)
        common_util.assert_in_error(self.family in public_key.curve_info.supported_families, "Family %s does not support the specified PEM %s curve type %s" % (self.family, public_pem, public_key.curve_info.name))
        common_util.assert_in_error(public_key.curve_info.enum == qky.keys[0].curve_info.enum, "QKY input %s (%s) does not match with key from PEM file %s (%s)" % (input_qky, qky.keys[0].curve_info.name, public_pem, public_key.curve_info.name))
        
        self.create_code_signing_key_entry(qky.qky, private_key, public_key, permission, cancel)
        
        # Delete
        del private_key
        del public_key
        
        # Write to file
        file = open(output_qky, "wb")
        qky.qky.data.tofile(file)
        file.close()
        
        del qky
        return True
        
    def insert_data(self, input_file, output_file, type) :
    
        reader = bitstream.get_family_reader(self.family, input_file, self.openssl, True, type, False, None)
               
        # Write it out
        file = open(output_file, "wb")
        reader.rbf.data.tofile(file)
        file.close()
        
        return True
        
    def sign(self, input_file, input_qky, private_pem, output_file, insert, type, cancel) :
    
        reader = bitstream.get_family_reader(self.family, input_file, self.openssl, insert, type, False, cancel)
        
        (qky, private_key) = self.get_qky_and_private_key(input_qky, private_pem)
        
        # Read and sign
        self.sign_bitstream(reader, qky, private_key)
       
        # Write it out
        file = open(output_file, "wb")
        reader.rbf.data.tofile(file)
        file.close()

        # memory
        del qky
        del private_key
        
        return True
        
    def root_key_hash(self, input_qky, output_text) :

        qky = keychain.QKY_READER(input_qky, self.openssl, self.family, None, None)
        common_util.assert_in_error(qky.complete == False, "Input %s should not have a complete signing chain" % input_qky)
        
        # Write to file
        file = open(output_text, "w")
        file.write("#################################################\n")
        file.write("#\n")
        file.write("# This is QKY root key\n")
        file.write("#\n")
        file.write("# QKY input: %s\n" % input_qky)
        file.write("#\n")
        file.write("#     Format (data is in hexadecimal character):\n")
        file.write("#\n")
        file.write("#        <Word #0> <Word #1> <Word #2> <Word #3> <Word #3> ... <Word #n-2> <Word #n-1>\n")
        file.write("#\n")
        file.write("#        Each word is 32bits or 4Bytes with following order\n")
        file.write("#\n")
        file.write("#           word = <Byte #3><Byte #2><Byte #1><Byte #0>\n")
        file.write("#\n")
        file.write("#           Example word = 12345678\n")
        file.write("#                   Byte #0 is 0x78\n")
        file.write("#                   Byte #1 is 0x56\n")
        file.write("#                   Byte #2 is 0x34\n")
        file.write("#                   Byte #3 is 0x12\n")
        file.write("#\n")
        file.write("#################################################\n\n")
        
        file.write("Root Key Hash: %s" % qky.root_key_hash)
        file.close()
        
        # Delete QKY
        del qky
        return True
        
    def get_qky_and_private_key(self, input_qky, private_pem) :
    
        # Read QKY
        qky = keychain.QKY_READER(input_qky, self.openssl, self.family, None, None)
        common_util.assert_in_error(len(qky.keys) >= 1, "QKY %s must have at least 1 entry, but found only %d entry" % (input_qky, len(qky.keys)))
        common_util.assert_in_error(qky.complete == False, "Input %s should not have a complete signing chain" % input_qky)
        
        # Read PEM
        private_key = self.key_manager.get_private_key(private_pem)
        common_util.assert_in_error(self.family in private_key.curve_info.supported_families, "Family %s does not support the specified PEM %s curve type %s" % (self.family, private_pem, private_key.curve_info.name))
        common_util.assert_in_error(private_key.curve_info.enum == qky.keys[0].curve_info.enum, "QKY input %s (%s) does not match with key from PEM file %s (%s)" % (input_qky, qky.keys[0].curve_info.name, private_pem, private_key.curve_info.name))
        private_key.match_xy(qky.keys[-1].key)
        
        return [qky, private_key]
        
    def sign_bitstream(self, rbf, qky, private_key) :

        # Make sure the chain requirement is good
        common_util.assert_in_error(len(qky.keys) >= 1, "QKY must have at least 1 entry, but found only %d entry" % (len(qky.keys)))
        if rbf.cancel == None :
            common_util.assert_in_error((len(qky.keys) - 1) >= rbf.database.CURRENT_TYPE.MIN_CODE_SIGNING_KEY_ENTRIES, "Family %s type %s must have at least %d Code Signing Key entry, but found %d" % (self.family, rbf.database.CURRENT_TYPE_NAME, rbf.database.CURRENT_TYPE.MIN_CODE_SIGNING_KEY_ENTRIES, len(qky.keys) - 1))
            common_util.assert_in_error((len(qky.keys) - 1) <= rbf.database.CURRENT_TYPE.MAX_CODE_SIGNING_KEY_ENTRIES, "Family %s type %s cannot have more than %d Code Signing Key entry, but found %d" % (self.family, rbf.database.CURRENT_TYPE_NAME, rbf.database.CURRENT_TYPE.MAX_CODE_SIGNING_KEY_ENTRIES, len(qky.keys) - 1))
        else :
            common_util.assert_in_error((len(qky.keys) - 1) == 0, "Family %s cancellation cert should contains root QKY but found %d Code Signing Key entry," % (self.family, len(qky.keys) - 1))

        permitted = False
        if rbf.is_permitted(qky.permission) :
            self.create_block0_entry(qky.qky, rbf.rbf.data[:128], private_key)
            rbf.rbf.assign_data(144, qky.qky.data)
            permitted = True
            
        common_util.assert_in_error(permitted, "QKY with permission (0x%08X) is not permitted to sign data type %s" % (qky.permission, rbf.type))
        
    def create_root_entry(self, data, public_key) :

        common_util.assert_in_error(data.size() == 0, "Root entry data should start with 0 Bytes, but found %d Bytes" % (data.size()))
        data.append_dword(database.ROOT_ENTRY_MAGIC_NUM)
        self.add_entry_key(data, public_key, -1, -1)
        common_util.assert_in_error(data.size() == 132, "Root entry data must be 132 Bytes, but found %d Bytes" % (data.size()))
        
        # Never exceed maximum size
        common_util.assert_in_error(data.size() <= self.database.SIGNATURE_MAX_SIZE, "Signing data (%d Bytes) exceeds maximum reserved size of %d Bytes" % (data.size(), self.database.SIGNATURE_MAX_SIZE))
        
    def add_entry_key(self, data, public_key, permission, cancel) :

        original_size = data.size()
        common_util.assert_in_error(cancel == -1 or cancel == 0xFFFFFFFF or cancel in self.database.SUPPORTED_CANCELS, "Cancel ID of 0x%08X is not supported by family %s" % (cancel, self.family))
        data.append_dword(public_key.curve_info.curve_magic_num)
        data.append_dword(permission)
        data.append_dword(cancel)
        # X
        data.append_data(public_key.xy.data[:public_key.curve_info.size])
        for i in range(public_key.curve_info.size, 48) :
            data.append_byte(0)
        # Y
        data.append_data(public_key.xy.data[public_key.curve_info.size:])
        for i in range(public_key.curve_info.size, 48) :
            data.append_byte(0)
        # Check size
        common_util.assert_in_error(data.size() == (original_size + 108), "Expect entry key data to be 128 Bytes, but found %d Bytes" % (data.size() - original_size + 20))
        # Pad to 128 Bytes
        while data.size() < (original_size + 128) :
            data.append_byte(0)

        # Never exceed maximum size
        common_util.assert_in_error(data.size() <= self.database.SIGNATURE_MAX_SIZE, "Signing data (%d Bytes) exceeds maximum reserved size of %d Bytes" % (data.size(), self.database.SIGNATURE_MAX_SIZE))
        
    def create_code_signing_key_entry(self, data, private_key, public_key, permission, cancel) :

        common_util.assert_in_error(private_key.curve_info.size == public_key.curve_info.size and \
                                (private_key.curve_info.size == 32 or private_key.curve_info.size == 48) and \
                                private_key.curve_info.name == public_key.curve_info.name, "Private key size and name (%d Bytes, %s) and public key size and name (%d Bytes, %s) does not match or invalid" % (private_key.curve_info.size, private_key.curve_info.name, public_key.curve_info.size, public_key.curve_info.name))
        common_util.assert_in_error(data.size() >= 132, "Data must be at least 132 Bytes of Root entry, but found %d Bytes" % (data.size()))
        common_util.assert_in_error(((data.size() - 132) % (232)) == 0, "Data must be at least 132 Bytes of Root entry and multiple of 232 Bytes Code Signing Key entry, but found %d Bytes" % (data.size()))
        code_signing_key_entry_count = int((data.size() - 132)/232)
        common_util.assert_in_error(code_signing_key_entry_count < self.database.MAX_CODE_SIGNING_KEY_ENTRIES, "Family %s can only allow maximum of %d Code Signing Key entry, unable to append more Code Signing Key entry to current key (%d)" % (self.family, self.database.MAX_CODE_SIGNING_KEY_ENTRIES, code_signing_key_entry_count))
        data.append_dword(database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM)
        signing_data_start_index = data.size()
        self.add_entry_key(data, public_key, permission, cancel)
        common_util.assert_in_error((data.size() - signing_data_start_index) == 128, "Key entry should be 128 Bytes, but found %d Bytes" % (data.size() - signing_data_start_index))
        self.sign_entry_signature(data, private_key, data.data[signing_data_start_index:signing_data_start_index+128])
        
        # Never exceed maximum size
        common_util.assert_in_error(data.size() <= self.database.SIGNATURE_MAX_SIZE, "Signing data (%d Bytes) exceeds maximum reserved size of %d Bytes" % (data.size(), self.database.SIGNATURE_MAX_SIZE))
        
    def sign_entry_signature(self, data, private_key, data_to_hash) :

        sha = self.openssl.get_byte_array_sha(private_key.curve_info.size, data_to_hash)
        data.append_dword(private_key.curve_info.sha_magic_num)
        private_key.sign(sha, data, 48)
        del sha
        
        # Never exceed maximum size
        common_util.assert_in_error(data.size() <= self.database.SIGNATURE_MAX_SIZE, "Signing data (%d Bytes) exceeds maximum reserved size of %d Bytes" % (data.size(), self.database.SIGNATURE_MAX_SIZE))
        
    def create_block0_entry(self, data, block0, private_key) :
    
        common_util.assert_in_error(data.size() >= 132, "Data must be at least 132 Bytes of Root entry, but found %d Bytes" % (data.size()))
        common_util.assert_in_error(((data.size() - 132) % (232)) == 0, "Data must be at least 132 Bytes of Root entry and multiple of 232 Bytes Code Signing Key entry, but found %d Bytes" % (data.size()))

        data.append_dword(database.BLOCK0_MAGIC_NUM)
        self.sign_entry_signature(data, private_key, block0)
        
        # Never exceed maximum size
        common_util.assert_in_error(data.size() <= self.database.SIGNATURE_MAX_SIZE, "Signing data (%d Bytes) exceeds maximum reserved size of %d Bytes" % (data.size(), self.database.SIGNATURE_MAX_SIZE))
        