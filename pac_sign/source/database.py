import common_util

class FILE_TYPE_DATABASE(object) :

    def __init__(self, min_csk, max_csk, enum, permission, supported_size=None) :
        self.MIN_CODE_SIGNING_KEY_ENTRIES = min_csk
        self.MAX_CODE_SIGNING_KEY_ENTRIES = max_csk
        common_util.assert_in_error(self.MAX_CODE_SIGNING_KEY_ENTRIES >= self.MIN_CODE_SIGNING_KEY_ENTRIES, "Impossible Code Signing Key entry count [min: %d, max: %d]" % (self.MIN_CODE_SIGNING_KEY_ENTRIES, self.MAX_CODE_SIGNING_KEY_ENTRIES))
        self.ENUM = enum
        self.PERMISSION = permission
        self.SUPPORTED_SIZE = supported_size

class FAMILY_DATABASE(object) :
    
    def __init__(self, family, supported_types, max_csk, signature_max_size, supported_cancels) :
    
        self.FAMILY = family
        self.SUPPORTED_TYPES = supported_types  # You can set the types to None, if this family has only one type data
        self.MAX_CODE_SIGNING_KEY_ENTRIES = max_csk
        self.SIGNATURE_MAX_SIZE = signature_max_size
        self.SUPPORTED_CANCELS = supported_cancels # You can set cancels to None, if it does not support cancel
        common_util.assert_in_error(self.SIGNATURE_MAX_SIZE > 0 and self.SIGNATURE_MAX_SIZE <= 880 and (self.SIGNATURE_MAX_SIZE % 4) == 0, "Maximum signature reserved field should greater than zero, less than 880 Bytes and multiple of 4 Bytes, but found %d" % self.SIGNATURE_MAX_SIZE)
        for key in self.SUPPORTED_TYPES :
            common_util.assert_in_error(self.SUPPORTED_TYPES[key].MIN_CODE_SIGNING_KEY_ENTRIES <= self.MAX_CODE_SIGNING_KEY_ENTRIES, "File type (%s) minimum Code Signing Key entry (%d) cannot be more than family (%s) maximum Code Signing Key entry (%d)" % (key, self.SUPPORTED_TYPES[key].MIN_CODE_SIGNING_KEY_ENTRIES, self.FAMILY, self.MAX_CODE_SIGNING_KEY_ENTRIES))
            common_util.assert_in_error(self.SUPPORTED_TYPES[key].MAX_CODE_SIGNING_KEY_ENTRIES <= self.MAX_CODE_SIGNING_KEY_ENTRIES, "File type (%s) maximum Code Signing Key entry (%d) cannot be more than family (%s) maximum Code Signing Key entry (%d)" % (key, self.SUPPORTED_TYPES[key].MAX_CODE_SIGNING_KEY_ENTRIES, self.FAMILY, self.MAX_CODE_SIGNING_KEY_ENTRIES))
        self.CURRENT_TYPE = None
        self.CURRENT_TYPE_NAME = None
        
    def get_type_from_enum(self, enum) :
    
        type = None
        for key in self.SUPPORTED_TYPES :
            if self.SUPPORTED_TYPES[key].ENUM == enum :
                self.CURRENT_TYPE = self.SUPPORTED_TYPES[key]
                self.CURRENT_TYPE_NAME = key
                type = key
                break
        return type

# For each supported family, the content type is very likely to be different as well as supported signing chain entry
FAMILY_LIST =   {
                    "PAC_CARD"      :   FAMILY_DATABASE("PAC_CARD", 
                                                        {
                                                            "FIM" : FILE_TYPE_DATABASE(1, 1, 0, 1), 
                                                            "BMC_FW" : FILE_TYPE_DATABASE(1, 1, 1, 2), 
                                                            "AFU" : FILE_TYPE_DATABASE(1, 1, 2, 4)
                                                        }, 1, 880, [i for i in range(0, 128)])
                }

# As long as we are still using the same crypto IP/FW the constant here should not change
# Define it here so that signer + keychain can access same data
DESCRIPTOR_BLOCK_MAGIC_NUM = 0xB6EAFD19
SIGNATURE_BLOCK_MAGIC_NUM = 0xF27F28D7
ROOT_ENTRY_MAGIC_NUM = 0xA757A046
CODE_SIGNING_KEY_ENTRY_MAGIC_NUM = 0x14711C2F
BLOCK0_MAGIC_NUM = 0x15364367
             