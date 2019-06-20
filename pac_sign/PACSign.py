##########################
#
# Main entry to the tool
#
##########################
import os
import sys
path = "%s/source/" % os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, path)
import common_util
import argparser
import bitstream
import sign

ARG_DB = {
            "arguments"     : [
                                argparser.ARGUMENT("permission",      "p",  argparser.ARGUMENT_TYPE.INT,  "permission value",
                                                    "Permission to authenticate"),
                                argparser.ARGUMENT("cancel",          "c",  argparser.ARGUMENT_TYPE.INT,  "cancel ID",
                                                    "Cancel ID"),
                                argparser.ARGUMENT("no_passphrase",   "s",  argparser.ARGUMENT_TYPE.BOOL, "",
                                                    "Disable passphrase for private PEM file"),
                                argparser.ARGUMENT("previous_qky",    "y",  argparser.ARGUMENT_TYPE.STR,  "previous QKY",
                                                    "Last entry of input QKY file"),
                                argparser.ARGUMENT("qky",             "q",  argparser.ARGUMENT_TYPE.STR,  "current QKY",
                                                    "Input QKY file"),
                                argparser.ARGUMENT("previous_pem",    "i",  argparser.ARGUMENT_TYPE.STR,  "previous PEM",
                                                    "Input PEM file of last entry of input QKY"),
                                argparser.ARGUMENT("pem",             "u",  argparser.ARGUMENT_TYPE.STR,  "current PEM",
                                                    "Input PEM file"),
                                argparser.ARGUMENT("curve",           "e",  argparser.ARGUMENT_TYPE.STR,  "secp256r1|secp384r1",
                                                    "Elliptic curve type"),
                                argparser.ARGUMENT("type",            "t",  argparser.ARGUMENT_TYPE.STR,  "FIM|BMC_FW|AFU",
                                                    "Bitstream content type")
                                ],
            "operations"    :   [
                                    argparser.OPERATION("MAKE_RAW_DATA", 
                                                        "generate raw content data file. The input is HEX string for example 0123456789ABCDEF",
                                                        [],
                                                        [],
                                                        2, 2, "<input HEX string> <output binary file>"),
                                    argparser.OPERATION("MAKE_PRIVATE_PEM", 
                                                        "generate private key",
                                                        ["curve"],
                                                        ["no_passphrase"],
                                                        1, 1, "<output PEM>"),
                                    argparser.OPERATION("MAKE_PUBLIC_PEM",
                                                        "generate public key",
                                                        [],
                                                        [],
                                                        2, 2, "<input private PEM> <output public PEM>"),
                                    argparser.OPERATION("MAKE_ROOT",
                                                        "generate root key which is QKY from a public root PEM file",
                                                        [],
                                                        [],
                                                        2, 2, "<public root PEM> <output QKY>"),
                                    argparser.OPERATION("APPEND_KEY",
                                                        "append new public key to QKY, which last entry private key must be known",
                                                        ["previous_pem", "previous_qky", "permission", "cancel"],
                                                        [],
                                                        2, 2, "<public PEM for new entry> <output QKY>"),
                                    argparser.OPERATION("INSERT_DATA",
                                                        "insert descriptor and signature block into input file",
                                                        ["type"],
                                                        [],
                                                        2, 2, "<input> <output>"),
                                    argparser.OPERATION("SIGN",
                                                        "sign bitstream with QKY, which last entry private key must be known",
                                                        ["qky", "pem"],
                                                        [],
                                                        2, 2, "<input> <output>"),
                                    argparser.OPERATION("INSERT_DATA_AND_SIGN",
                                                        "insert descriptor and signature block into input file, sign file with QKY, which last entry private key must be known",
                                                        ["type", "qky", "pem"],
                                                        [],
                                                        2, 2, "<input> <output>"),
                                    argparser.OPERATION("MAKE_AND_SIGN_CANCELLATION_CERT",
                                                        "generate cancellation cert with the specified cancel ID (4 Bytes) and sign with root QKY which private key must be known",
                                                        ["type", "qky", "pem", "cancel"],
                                                        [],
                                                        1, 1, "<output cert>"),
                                    argparser.OPERATION("ROOT_KEY_HASH",
                                                        "extract Root Key Hash information from QKY",
                                                        [],
                                                        [],
                                                        2, 2, "<input QKY> <output fuse text>"),
                                    argparser.OPERATION("CHECK_INTEGRITY",
                                                        "check the integrity of the file",
                                                        [],
                                                        [],
                                                        1, 1, "<input>")
                                ]
        }

def main() :

    (operation, permission, cancel, no_passphrase, previous_qky, qky, previous_pem, pem, curve, type, help, args) = argparser.get_argument(ARG_DB)
    
    status = None
    if help :
    
        # argparser will print for you
        pass
    
    else :
    
        if operation == "MAKE_RAW_DATA" :
            data = common_util.BYTE_ARRAY("HEXSTRING", args[0])
            file = open(args[1], "wb")
            data.data.tofile(file)
            file.close()
            del data
            status = True
        else :
            signer = sign.SIGNER("PAC_CARD")
            if operation == "MAKE_PRIVATE_PEM" :
                if len(args) == 1 and curve != None :
                    common_util.assert_in_error((curve == "secp256r1" or curve == "secp384r1"), "--curve must be secp256r1 or secp384r1")
                    common_util.assert_in_error(common_util.check_extension(args[0], ".pem"), "Output file %s extension must be \".pem\"" % args[0])
                    status = signer.make_private_pem(curve, args[0], no_passphrase == None)
            elif operation == "MAKE_PUBLIC_PEM" :
                if len(args) == 2 :
                    common_util.assert_in_error(common_util.check_extension(args[0], ".pem"), "Input file %s extension must be \".pem\"" % args[0])
                    common_util.assert_in_error(common_util.check_extension(args[1], ".pem"), "Output file %s extension must be \".pem\"" % args[1])
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    status = signer.make_public_pem(args[0], args[1])
            elif operation == "MAKE_ROOT" :
                if len(args) == 2 :
                    common_util.assert_in_error(common_util.check_extension(args[0], ".pem"), "Input file %s extension must be \".pem\"" % args[0])
                    common_util.assert_in_error(common_util.check_extension(args[1], ".qky"), "Output file %s extension must be \".qky\"" % args[1])
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    status = signer.make_root(args[0], args[1])
            elif operation == "APPEND_KEY" :
                if len(args) == 2 and previous_pem != None and previous_qky != None :
                    common_util.assert_in_error(common_util.check_extension(previous_pem, ".pem"), "--previous_pem file %s extension must be \".pem\"" % previous_pem)
                    common_util.assert_in_error(common_util.check_extension(previous_qky, ".qky"), "--previous_qky file %s extension must be \".qky\"" % previous_qky)
                    common_util.assert_in_error(common_util.check_extension(args[0], ".pem"), "Input file %s extension must be \".pem\"" % args[0])
                    common_util.assert_in_error(common_util.check_extension(args[1], ".qky"), "Output file %s extension must be \".qky\"" % args[1])
                    common_util.assert_in_error(os.path.exists(previous_pem), "File %s does not exist" % previous_pem)
                    common_util.assert_in_error(os.path.exists(previous_qky), "File %s does not exist" % previous_qky)
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    status = signer.append_key(previous_qky, previous_pem, args[0], args[1], permission, cancel)
            elif operation == "INSERT_DATA" :
                if len(args) == 2 :
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    status = signer.insert_data(args[0], args[1], type)
            elif operation == "SIGN" or operation == "INSERT_DATA_AND_SIGN" :
                if len(args) == 2 and qky != None and pem != None :
                    common_util.assert_in_error(common_util.check_extension(qky, ".qky"), "--qky file %s extension must be \".qky\"" % qky)
                    common_util.assert_in_error(common_util.check_extension(pem, ".pem"), "--pem file %s extension must be \".pem\"" % pem)
                    common_util.assert_in_error(os.path.exists(qky), "File %s does not exist" % qky)
                    common_util.assert_in_error(os.path.exists(pem), "File %s does not exist" % pem)
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    status = signer.sign(args[0], qky, pem, args[1], operation == "INSERT_DATA_AND_SIGN", type, None)
            elif operation == "MAKE_AND_SIGN_CANCELLATION_CERT" :
                if len(args) == 1 and type != None and qky != None and pem != None and cancel != None:
                    common_util.assert_in_error(common_util.check_extension(qky, ".qky"), "--qky file %s extension must be \".qky\"" % qky)
                    common_util.assert_in_error(common_util.check_extension(pem, ".pem"), "--pem file %s extension must be \".pem\"" % pem)
                    common_util.assert_in_error(common_util.check_extension(args[0], ".cert"), "Output cert %s extension must be \".cert\"" % args[0])
                    common_util.assert_in_error(os.path.exists(qky), "File %s does not exist" % qky)
                    common_util.assert_in_error(os.path.exists(pem), "File %s does not exist" % pem)
                    status = signer.sign(None, qky, pem, args[0], True, type, cancel)
            elif operation == "ROOT_KEY_HASH" :
                if len(args) == 2 :
                    common_util.assert_in_error(common_util.check_extension(args[0], ".qky"), "Input file %s extension must be \".qky\"" % args[0])
                    common_util.assert_in_error(common_util.check_extension(args[1], ".txt"), "Output file %s extension must be \".txt\"" % args[1])
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    status = signer.root_key_hash(args[0], args[1])
            elif operation == "CHECK_INTEGRITY" :
                if len(args) == 1 :
                    common_util.assert_in_error(os.path.exists(args[0]), "File %s does not exist" % args[0])
                    reader = bitstream.get_family_reader("PAC_CARD", args[0], signer.openssl, False, None, True, None)
                    status = True
            else :
                common_util.assert_in_error(False, "Invalid specified operation %s" % operation)
            del signer

    if status == True :
        return 0
    else :
        return (-1)        

if __name__ == "__main__":
    main()
