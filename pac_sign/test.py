import os

assert os.system("rm -rf test") == 0
assert os.system("mkdir test") == 0
# print ("Generate two pairs of private/public key")
assert os.system(
    ("python PACSign.py --operation=make_private_pem " +
     "--curve=secp384r1 --no_passphrase test/pri1.pem -k key_manager")) != 0
assert os.system(
    ("python PACSign.py --operation=make_private_pem " +
     "--curve=secp384r1 --no_passphrase test/pri2.pem -k key_manager")) == 0
assert os.system(
    "python PACSign.py --operation=make_private_pem " +
    "--curve=secp256r1 --no_passphrase test/pri3.pem -k key_manager") == 0
assert os.system(
    "python PACSign.py --operation=make_private_pem " +
    "--curve=secp384r1 --no_passphrase test/pri4.pem -k key_manager") == 0
assert os.system("python PACSign.py --operation=make_public_pem " +
                 "test/pri1.pem test/pub1.pem -k key_manager") == 0
assert os.system("python PACSign.py --operation=make_public_pem " +
                 "test/pri2.pem test/pub2.pem -k key_manager") == 0
assert os.system("python PACSign.py --operation=make_public_pem " +
                 "test/pri3.pem test/pub3.pem -k key_manager") == 0
assert os.system("python PACSign.py --operation=make_public_pem " +
                 "test/pri4.pem test/pub4.pem -k key_manager") == 0
print("Generate root keychain")
assert os.system("python PACSign.py --operation=make_root test/pub1.pem " +
                 "test/root.qky -k key_manager") == 0
print("Append new key to root keychain to generate new keychain")
assert os.system(
    "python PACSign.py --operation=append_key --permission=-1 " +
    "--cancel=0 --previous_qky=test/root.qky --previous_pem=test/pri1.pem " +
    "test/pub2.pem test/key.qky -k key_manager") == 0
print("Append new key to root keychain to generate new keychain " +
      "(negative test)")
assert os.system(
    "python PACSign.py --operation=append_key --permission=-1 " +
    "--cancel=0 --previous_qky=test/key.qky --previous_pem=test/pri2.pem " +
    "test/pub4.pem test/negative.qky -k key_manager") != 0
assert os.system(
    "python PACSign.py --operation=append_key --permission=-1 " +
    "--cancel=0 --previous_qky=test/root.qky --previous_pem=test/pri1.pem " +
    "test/pub3.pem test/negative.qky -k key_manager") != 0
print("Insert Block0/Block1 into raw data and sign")
assert (
    os.system("python PACSign.py FIM -t update -H openssl_manager --yes " +
              "-i hello_mem_afu.gbs -o s_PACSign.py " +
              "-r d:/keys/darby/darby_dev_fim_root_public_256.pem " +
              "-k d:/keys/darby/darby_dev_fim_csk0_public_256.pem -vv") == 0)
print("************************Insert Block0/Block1 into raw data and sign")
assert (os.system(
    "python PACSign.py FIM -t update -H openssl_manager  -i PACSign.py " +
    "-o us_PACSign.py -vv -y") == 0)
assert (os.system(
    "python PACSign.py FIM -t update -H pkcs11_manager -k csk0 -r root_key " +
    "-i PACSign.py -o us_pkcs11_PACSign.py -C PKCS11_config.json -y -vvv") == 0
        )
print("Insert Block0/Block1 into raw data and sign (negative test)")
assert (os.system(
    "python PACSign.py --operation=insert_data_and_sign --type=FIM " +
    "--qky=test/root.qky --pem=test/pri1.pem data.bin " +
    "test/negative.bin -k key_manager -x update") != 0)
print("Insert Block0/Block1 into raw data, the output is unsigned data")
assert (os.system(
    "python PACSign.py --operation=insert_data --type=BMC_FW data.bin " +
    "test/unsigned_data.bin -k key_manager -x update") == 0)
print("Sign the unsigned data")
assert (os.system(
    "python PACSign.py --operation=sign --qky=test/key.qky " +
    "--pem=test/pri2.pem test/unsigned_data.bin test/data2sign.bin " +
    "-k key_manager -x update") == 0)
print("Read Root Key Hash")
assert (
    os.system("python PACSign.py --operation=root_key_hash test/root.qky " +
              "test/root.txt -k key_manager") == 0)
assert (os.system("python PACSign.py --operation=root_key_hash test/key.qky " +
                  "test/key.txt -k key_manager") == 0)
print("Check file integrity")
assert (os.system(
    "python PACSign.py --operation=check_integrity test/unsigned_data.bin" +
    " > test/unsigned_data.bin.txt -k key_manager") == 0)
assert (os.system(
    "python PACSign.py --operation=check_integrity test/data1sign.bin " +
    "> test/data1sign.bin.txt -k key_manager") == 0)
assert (os.system(
    "python PACSign.py --operation=check_integrity test/data2sign.bin " +
    "> test/data2sign.bin.txt -k key_manager") == 0)
print("Make and sign cancellation cert")
assert (os.system(
    "python PACSign.py --operation=make_and_sign_cancellation_cert " +
    "--type=FIM --qky=test/root.qky --pem=test/pri1.pem " +
    "--cancel=1 test/cancel.cert -k key_manager") == 0)
print("Make and sign cancellation cert (negative test)")
assert (os.system(
    "python PACSign.py --operation=make_and_sign_cancellation_cert " +
    "--type=FIM --qky=test/root.qky --pem=test/pri1.pem " +
    "--cancel=189 test/cancel.cert -k key_manager") != 0)
assert (os.system(
    "python PACSign.py --operation=make_and_sign_cancellation_cert " +
    "--type=FIM --qky=test/key.qky --pem=test/pri2.pem " +
    "--cancel=1 test/negative.cert -k key_manager") != 0)
assert (os.system(
    "python PACSign.py --operation=make_and_sign_cancellation_cert " +
    "--type=FIM --qky=test/key.qky --pem=test/pri2.pem " +
    "--cancel=1 test/negative.bin -k key_manager") != 0)
print("Check cancellation cert integrity")
assert (os.system(
    "python PACSign.py --operation=check_integrity test/cancel.cert " +
    "> test/cancel.cert.txt -k key_manager") == 0)
assert not os.path.exists("test/negative.qky")
assert not os.path.exists("test/negative.bin")
assert not os.path.exists("test/negative.cert")
print("Misc")
assert os.system("python PACSign.py --help > test/help.txt") == 0
assert os.system(
    "python PACSign.py --help --operation=sign  > test/ohelp.txt") == 0
