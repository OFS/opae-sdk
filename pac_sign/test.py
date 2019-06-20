import os
assert os.system("rm -rf test") == 0
assert os.system("mkdir test") == 0
print ("Generate two pairs of private/public key")
assert os.system("python PACSign.py --operation=make_private_pem --curve=secp384r1 --no_passphrase test/pri1.pem") == 0
assert os.system("python PACSign.py --operation=make_private_pem --curve=secp384r1 --no_passphrase test/pri2.pem") == 0
assert os.system("python PACSign.py --operation=make_private_pem --curve=secp256r1 --no_passphrase test/pri3.pem") == 0
assert os.system("python PACSign.py --operation=make_private_pem --curve=secp384r1 --no_passphrase test/pri4.pem") == 0
assert os.system("python PACSign.py --operation=make_public_pem test/pri1.pem test/pub1.pem") == 0
assert os.system("python PACSign.py --operation=make_public_pem test/pri2.pem test/pub2.pem") == 0
assert os.system("python PACSign.py --operation=make_public_pem test/pri3.pem test/pub3.pem") == 0
assert os.system("python PACSign.py --operation=make_public_pem test/pri4.pem test/pub4.pem") == 0
print ("Generate root keychain")
assert os.system("python PACSign.py --operation=make_root test/pub1.pem test/root.qky") == 0
print ("Append new key to root keychain to generate new keychain")
assert os.system("python PACSign.py --operation=append_key --permission=-1 --cancel=0 --previous_qky=test/root.qky --previous_pem=test/pri1.pem test/pub2.pem test/key.qky") == 0
print ("Append new key to root keychain to generate new keychain (negative test)")
assert os.system("python PACSign.py --operation=append_key --permission=-1 --cancel=0 --previous_qky=test/key.qky --previous_pem=test/pri2.pem test/pub4.pem test/negative.qky") != 0
assert os.system("python PACSign.py --operation=append_key --permission=-1 --cancel=0 --previous_qky=test/root.qky --previous_pem=test/pri1.pem test/pub3.pem test/negative.qky") != 0
print ("Insert Block0/Block1 into raw data and sign")
assert os.system("python PACSign.py --operation=insert_data_and_sign --type=FIM --qky=test/key.qky --pem=test/pri2.pem data.bin test/data1sign.bin") == 0
print ("Insert Block0/Block1 into raw data and sign (negative test)")
assert os.system("python PACSign.py --operation=insert_data_and_sign --type=FIM --qky=test/root.qky --pem=test/pri1.pem data.bin test/negative.bin") != 0
print ("Insert Block0/Block1 into raw data, the output is unsigned data")
assert os.system("python PACSign.py --operation=insert_data --type=BMC_FW data.bin test/unsigned_data.bin") == 0
print ("Sign the unsigned data")
assert os.system("python PACSign.py --operation=sign --qky=test/key.qky --pem=test/pri2.pem test/unsigned_data.bin test/data2sign.bin") == 0
print ("Read Root Key Hash")
assert os.system("python PACSign.py --operation=root_key_hash test/root.qky test/root.txt") == 0
assert os.system("python PACSign.py --operation=root_key_hash test/key.qky test/key.txt") == 0
print ("Check file integrity")
assert os.system("python PACSign.py --operation=check_integrity test/unsigned_data.bin > test/unsigned_data.bin.txt") == 0
assert os.system("python PACSign.py --operation=check_integrity test/data1sign.bin > test/data1sign.bin.txt") == 0
assert os.system("python PACSign.py --operation=check_integrity test/data2sign.bin > test/data2sign.bin.txt") == 0
print ("Make and sign cancellation cert")
assert os.system("python PACSign.py --operation=make_and_sign_cancellation_cert --type=FIM --qky=test/root.qky --pem=test/pri1.pem --cancel=1 test/cancel.cert") == 0
print ("Make and sign cancellation cert (negative test)")
assert os.system("python PACSign.py --operation=make_and_sign_cancellation_cert --type=FIM --qky=test/root.qky --pem=test/pri1.pem --cancel=189 test/cancel.cert") != 0
assert os.system("python PACSign.py --operation=make_and_sign_cancellation_cert --type=FIM --qky=test/key.qky --pem=test/pri2.pem --cancel=1 test/negative.cert") != 0
assert os.system("python PACSign.py --operation=make_and_sign_cancellation_cert --type=FIM --qky=test/key.qky --pem=test/pri2.pem --cancel=1 test/negative.bin") != 0
print ("Check cancellation cert integrity")
assert os.system("python PACSign.py --operation=check_integrity test/cancel.cert > test/cancel.cert.txt") == 0
assert os.path.exists("test/negative.qky") == False
assert os.path.exists("test/negative.bin") == False
assert os.path.exists("test/negative.cert") == False
print ("Misc")
assert os.system("python PACSign.py --help > test/help.txt") == 0
assert os.system("python PACSign.py --help --operation=sign  > test/ohelp.txt") == 0