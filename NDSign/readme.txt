

-----------------------------------------------------
 How to sign with a usb token using ltsign utility
-----------------------------------------------------

1.	Get your normal qshell env and build your decompress cmf in whatever way you currently do this

2.	Add the css resource and the nios2test resource to your env by doing:
% arc shell css nios2test

3.	p4 sync (to the head revision) and cd to the root directory where the ndsign utility lives:
% p4 sync //acds/main/quartus/devices/firmware/tools/sign/...
% cd ${ACDS_SRC_ROOT}/quartus/devices/firmware/tools/sign

4.	use --help to see how the tool works:
% ./ndsign.py --help
% ./ndsign.py create_root_key --help

5.	Generate a root key:
% mkdir keys
% ./ndsign.py create_root_key keys/root.qky

6.	Generate and sign a codesign key using that root key from step 5
% ./ndsign.py sign_key keys/root.qky keys/codesign1.qky

7.	Sign the cmf you created in step 1 with the codesign key you signed in step 6
% ./ndsign.py sign_cmf keys/codesign1.qky <your_cmf> signed.cmf

8.	For sanity purposes, test your signed cmf from step 7 with Eamon's tool:
% /tools/soc/clarke/bootrom/bin/REVB/checkwrap -v -v -v -i signed.cmf
