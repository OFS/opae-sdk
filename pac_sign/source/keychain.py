##########################
#
# Interpret our QKY 
#
##########################
import common_util
import openssl
import key_manager
import database

class QKY_READER :

    def __init__(self, filename, openssl, family, external_data, block0, write_individual_entry_filename=None) :
    
        common_util.assert_in_error(family in database.FAMILY_LIST, "Supported family is %s, but found %s" % (database.FAMILY_LIST.keys(), family))
        self.family = family
        self.database = database.FAMILY_LIST[family]
        
        if len(filename) :
            assert external_data == None
            self.qky = common_util.BYTE_ARRAY("FILE", filename)
        else :
            assert external_data != None and len(external_data)
            self.qky = common_util.BYTE_ARRAY()
            self.qky.append_data(external_data)
        common_util.assert_in_error(self.qky.size() > 0 and (self.qky.size() % 4) == 0, "QKY data size should be greater than zeroa and multiple of 4 Bytes, but found %d Bytes" % self.qky.size())
        
        self.offset = 0
        self.key_offset = 0
        self.root_key_hash = ""
        self.keys = []
        self.complete = False
        self.permission = 0xFFFFFFFF
        self.openssl = openssl
        self.block0 = block0
        
        # There must at least root key
        (permission, cancel) = self.get_root_entry()
        common_util.assert_in_error(permission == 0xFFFFFFFF and cancel == 0xFFFFFFFF, "Root key permission and cancel must be both 0xFFFFFFFF, but found 0x%08X and 0x%08X respectively" % (permission, cancel))
        write_individual_entry_offset = None
        if write_individual_entry_filename != None and len(write_individual_entry_filename) :
            self.write_ltsign_entry("%s.root_entry.bin" % write_individual_entry_filename, None, self.qky.data[:self.offset])
            write_individual_entry_offset = self.offset
        complete = False
        csk_index = 0
        while self.offset < self.qky.size() :
            magic_number = self.qky.get_dword(self.offset)
            common_util.assert_in_error(magic_number == database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM or magic_number == database.BLOCK0_MAGIC_NUM, "Non-Root entry can only be 0x%08X or 0x%08X, but found 0x%08X" % (database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM, database.BLOCK0_MAGIC_NUM, magic_number))
            if magic_number == database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM :
                (permission, cancel) = self.get_code_signing_key_entry()
                if self.database.SUPPORTED_TYPES == None :
                    common_util.assert_in_error(permission == 0xFFFFFFFF, "Family %s does not support bitstream type, permission should be 0xFFFFFFFF, but found 0x%08X" % (self.family, permission))
                if self.database.SUPPORTED_CANCELS == None :
                    common_util.assert_in_error(cancel == 0xFFFFFFFF, "Family %s does not support cancellation, cancel ID should be 0xFFFFFFFF, but found 0x%08X" % (self.family, cancel))
                else :
                    common_util.assert_in_error(cancel in self.database.SUPPORTED_CANCELS, "Code Signing Key entry cancel ID of 0x%08X is not supported by family %s" % (cancel, self.family))
                self.permission &= permission
                if write_individual_entry_filename != None and len(write_individual_entry_filename) :
                    self.write_ltsign_entry("%s.csk_entry%d.bin" % (write_individual_entry_filename, csk_index), None, self.qky.data[write_individual_entry_offset:self.offset])
                    write_individual_entry_offset = self.offset
                    csk_index += 1
            else :
                chain_end_index = self.get_block0_entry()
                self.complete = True
                if write_individual_entry_filename != None and len(write_individual_entry_filename) :
                    self.write_ltsign_entry("%s.block0_entry.bin" % write_individual_entry_filename, block0, self.qky.data[write_individual_entry_offset:chain_end_index])
                    write_individual_entry_offset = self.offset
        common_util.assert_in_error(self.offset == self.qky.size(), "Invalid QKY")
        common_util.assert_in_error(len(self.keys), "QKY must have at least one entry")
        common_util.assert_in_error(self.family in self.keys[0].curve_info.supported_families, "Family %s does not support the specified QKY %s curve type %s" % (self.family, filename, self.keys[0].curve_info.name))
        
        if write_individual_entry_filename != None and len(write_individual_entry_filename) and self.complete :
            key_index = 0
            for key in self.keys :
                if key_index < (len(self.keys) - 1) :
                    self.write_ltsign_key("%s.csk_entry%d.verify.kp" % (write_individual_entry_filename, key_index), key)
                else :
                    self.write_ltsign_key("%s.block0.verify.kp" % write_individual_entry_filename, key)
                key_index += 1
                
        
    def __del__(self) :
    
        self.clean()
    
    def __exit__(self) :
    
        self.clean()
        
    def clean(self) :
    
        while len(self.keys) :
            del self.keys[-1]
        self.keys = [] 
        
    def write_ltsign_entry(self, filename, additional_data, entry_data) :
    
        common_util.print_info("Writing %s" % filename)
        file = open(filename, "wb")
        temp = common_util.BYTE_ARRAY()
        if additional_data != None :
            temp.append_data(additional_data)
        temp.append_data(entry_data)
        temp.data.tofile(file)
        file.close()
        del temp
        
    def write_ltsign_key(self, filename, key) :
    
        common_util.print_info("Writing verify key %s (This key can only be used for verification for now but not signing)" % filename)
        file = open(filename, "wb")
        temp = common_util.BYTE_ARRAY()
        strings = []
        if key.curve_info.name == "prime256v1" :
            strings.append("(8:sequence(10:public-key(22:ecdsa-secp256r1-sha256(2:qx32:")
            strings.append(")(2:qy32:")
            strings.append(")))(11:private-key(22:ecdsa-secp256r1-sha256(1:d32:")
            strings.append("))))")
        elif key.curve_info.name == "secp384r1" :
            strings.append("(8:sequence(10:public-key(22:ecdsa-secp384r1-sha384(2:qx48:")
            strings.append(")(2:qy48:")
            strings.append(")))(11:private-key(22:ecdsa-secp384r1-sha384(1:d48:")
            strings.append("))))")
        else :
            common_util.assert_in_error(False, "Does not support LTSign Key format for %s" % key.curve_info.name)
        temp.append_data(strings[0])
        temp.append_data(key.xy.data[:key.curve_info.size])
        temp.append_data(strings[1])
        temp.append_data(key.xy.data[key.curve_info.size:])
        temp.append_data(strings[2])
        for i in range(key.curve_info.size) :
            temp.append_byte(0)
        temp.append_data(strings[3])
        temp.data.tofile(file)
        file.close()
        del temp
        
    def get_root_entry(self) :
    
        common_util.assert_in_error(self.offset == 0, "Root entry must start from offset 0, but found %d" % self.offset)
        common_util.assert_in_error(self.qky.size() >= 132, "QKY size should be at least 132 Bytes to read Root entry, but found %d Bytes" % self.qky.size())
        magic_num = self.qky.get_dword(self.offset)
        common_util.assert_in_error(magic_num == database.ROOT_ENTRY_MAGIC_NUM, "Root entry magic num should be 0x%08X, but found 0x%08X" % (database.ROOT_ENTRY_MAGIC_NUM, magic_num))
        self.offset += 4
        return self.get_key()
        
    def get_code_signing_key_entry(self) :
    
        common_util.assert_in_error(self.offset >= 132, "Code Signing Key entry must at least start from offset 132, but found %d" % self.offset)
        common_util.assert_in_error(self.qky.size() >= self.offset + 232, "QKY size should be at least %d Bytes to read Code Signing Key entry from offset %d, but found %d Bytes" % (self.offset + 232, self.offset, self.qky.size()))
        common_util.assert_in_error((self.offset - 132) % 232 == 0, "QKY offset must be at least 132 Bytes of Root entry and multiple of 232 Bytes Code Signing Key entry, but found %d Bytes" % (self.offset))
        code_signing_key_entry_count = int((self.offset - 132)/232)
        common_util.assert_in_error((code_signing_key_entry_count + 1) == len(self.keys), "Code Signing Key entry count (%d) does not match with Public Key count (%d - 1) [First key is Root entry public key]" % (code_signing_key_entry_count, len(self.keys)))
        common_util.assert_in_error(code_signing_key_entry_count < self.database.MAX_CODE_SIGNING_KEY_ENTRIES, "Unable to append key as Code Signing Key entry count (%d) has reached the maximum allow entires (%d)" % (code_signing_key_entry_count, self.database.MAX_CODE_SIGNING_KEY_ENTRIES))
        magic_num = self.qky.get_dword(self.offset)
        common_util.assert_in_error(magic_num == database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM, "Code Signing Key entry magic num should be 0x%08X, but found 0x%08X" % (database.CODE_SIGNING_KEY_ENTRY_MAGIC_NUM, magic_num))
        self.offset += 4
        signing_data_start_index = self.offset        
        permission_cancel = self.get_key()
        common_util.assert_in_error(len(self.keys) >= 2, "There must be at least two keys in list when verifying Code Signing Key entry, but found %d key" % len(self.keys))
        common_util.assert_in_error((self.offset - signing_data_start_index) == 128, "Key entry should be 128 Bytes, but found %d Bytes" % (self.offset - signing_data_start_index))
        self.signature_verification(self.keys[-2], self.qky.data[signing_data_start_index:signing_data_start_index+128])
        return permission_cancel
        
    def get_block0_entry(self) :
    
        common_util.assert_in_error(self.offset >= 132, "Block0 entry must at least start from offset 132, but found %d" % self.offset)
        common_util.assert_in_error(self.qky.size() >= self.offset + 104, "QKY size should be at least %d Bytes to read Block0 entry from offset %d, but found %d Bytes" % (self.offset + 104, self.offset, self.qky.size()))
        common_util.assert_in_error((self.offset - 132) % 232 == 0, "QKY offset must be at least 132 Bytes of Root entry and multiple of 232 Bytes Code Signing Key entry, but found %d Bytes" % (self.offset))
        code_signing_key_entry_count = int((self.offset - 132)/232)
        common_util.assert_in_error((code_signing_key_entry_count + 1) == len(self.keys), "Code Signing Key entry count (%d) does not match with Public Key count (%d - 1) [First key is Root entry public key]" % (code_signing_key_entry_count, len(self.keys)))
        magic_num = self.qky.get_dword(self.offset)
        common_util.assert_in_error(magic_num == database.BLOCK0_MAGIC_NUM, "Block0 entry magic num should be 0x%08X, but found 0x%08X" % (database.BLOCK0_MAGIC_NUM, magic_num))
        self.offset += 4
        
        common_util.assert_in_error(len(self.keys) >= 1, "There must be at least one key in list when verifying Block0 entry, but found %d key" % len(self.keys))
        common_util.assert_in_error(self.block0 != None, "There must be Block0 data to be verified in Block0 entry")
        self.signature_verification(self.keys[-1], self.block0)
        chain_end_index = self.offset
        
        while self.offset < self.qky.size() :
            common_util.assert_in_error(self.qky.data[self.offset] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (self.offset, self.qky.data[self.offset]))
            self.offset += 1
            
        return chain_end_index
        
    def get_key(self) :
    
        common_util.assert_in_error((self.offset + 128) <= self.qky.size(), "Unable to read public key at offset (%d), which exceeds QKY size (%d Bytes)" % (self.offset, self.qky.size()))
        self.key_offset = self.offset

        # Curve
        curve_magic_number = self.qky.get_dword(self.offset)
        curve_info = openssl.get_curve_info_from_curve_magic_num(curve_magic_number)
        common_util.assert_in_error(curve_info != None, "Expects curve magic number to be %s but found 0x%08X" % (openssl.get_supported_curve_info_curve_magic_nums(), curve_magic_number))
        # Permission
        permission = self.qky.get_dword(self.offset + 4)
        # Cancel
        cancel = self.qky.get_dword(self.offset + 8)
        # XY
        xy = []
        for i in range(self.offset + 12, self.offset + 12 + curve_info.size) :
            xy.append(self.qky.data[i])
        for i in range(self.offset + 12 + curve_info.size, self.offset + 12 + 48) :
            common_util.assert_in_error(self.qky.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.qky.data[i]))
        for i in range(self.offset + 12 + 48, self.offset + 12 + 48 + curve_info.size) :
            xy.append(self.qky.data[i])
        for i in range(self.offset + 12 + 48 + curve_info.size, self.offset + 12 + 48 + 48) :
            common_util.assert_in_error(self.qky.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.qky.data[i]))
        # Reserved
        for i in range(self.offset + 108, self.offset + 128) :
            common_util.assert_in_error(self.qky.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.qky.data[i]))
        key = self.openssl.generate_ec_key_using_xy_and_curve_info(xy, curve_info)
        key = key_manager.PUBLIC_KEY(key, self.openssl)
        if len(self.keys) :
            common_util.assert_in_error(self.keys[-1].curve_info.name == curve_info.name, "Found mix curve in QKY, Public key #%d (%s) mismatch with Public key #%d (%s)" % (len(self.keys)-1, self.keys[-1].curve_info.name, len(self.keys), curve_info.name))
            common_util.assert_in_error(len(self.root_key_hash), "Missing fuse info")
        else :
            common_util.assert_in_error(len(self.root_key_hash) == 0, "There should not be any fuse info yet")
            common_util.assert_in_error(self.offset == 4, "Offset should be 4 when getting fuse info, but found %d" % self.offset)
            sha = self.openssl.get_byte_array_sha(curve_info.size, self.qky.data[4:132])
            for i in range(int(curve_info.size/4)) :
                self.root_key_hash = "%s %08X" % (self.root_key_hash, sha.get_dword(i * 4))
            self.root_key_hash = self.root_key_hash[1:]
            del sha
        self.keys.append(key)
        # Update offset
        self.offset += 128
        return [permission, cancel]
        
    def signature_verification(self, key, data_to_be_verified) :
    
        common_util.assert_in_error((self.offset + 100) <= self.qky.size(), "Unable to read signature at offset (%d), which exceeds QKY size (%d Bytes)" % (self.offset, self.qky.size()))
        signature_hash_magic = self.qky.get_dword(self.offset)
        supported_sha_magic_nums = openssl.get_supported_curve_info_sha_magic_nums()
        common_util.assert_in_error(("0x%08X" % signature_hash_magic) in supported_sha_magic_nums, "Signature hash magic num should be %s, but found 0x%08X" % (supported_sha_magic_nums, signature_hash_magic))
        if signature_hash_magic != key.curve_info.sha_magic_num :
            common_util.print_warning("Signature hash magic num (0x%08X) from file does not align with curve %s expectation (0x%08X)" % (signature_hash_magic, key.curve_info.name, key.curve_info.sha_magic_num))
        sha_size = openssl.get_sha_magic_num_size(signature_hash_magic)
        common_util.assert_in_error(sha_size != None, "Signature hash magic num (0x%08X) does not have valid size" % signature_hash_magic)
        sha = self.openssl.get_byte_array_sha(sha_size, data_to_be_verified)
        rs = []
        for i in range(self.offset + 4, self.offset + 4 + key.curve_info.size) :
            rs.append(self.qky.data[i])
        for i in range(self.offset + 4 + key.curve_info.size, self.offset + 4 + 48) :
            common_util.assert_in_error(self.qky.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.qky.data[i]))
        for i in range(self.offset + 4 + 48, self.offset + 4 + 48 + key.curve_info.size) :
            rs.append(self.qky.data[i])
        for i in range(self.offset + 4 + 48 + key.curve_info.size, self.offset + 4 + 48 + 48) :
            common_util.assert_in_error(self.qky.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.qky.data[i]))
        key.verify_signature(sha, rs)
        del sha
        self.offset += 100
        
    def get_rid_extra_data(self) :
    
        # NULL before delete/pop
        if self.offset < self.qky.size() :
            size_to_null = self.qky.size() - self.offset
            for i in range(size_to_null) :
                self.qky.data[self.offset + i] = 0
                
        while self.offset < self.qky.size() :
            self.qky.data.pop()