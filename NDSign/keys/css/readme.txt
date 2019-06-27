Key Vault Logs

============================
Engineering and Manufacturing Keychains
============================
Re-create engineering and manufacturing keychains (hacked sign/css.py; see: https://wiki.ith.intel.com/display/PDFI/NDSign+FAQ#NDSignFAQ-Howtorecreatekeychainwithoutgoingthroughsignceremony?).
There was a change to only support ND format in ND firmware. As a result, ND single root format is changed slightly. 

% python ndsign.py create_root_key --hash-sel=2 --keyfile=keys/css/StratixEngineeringRootKey_Release_pubkey.bin StratixEngineeringRootKey.qky 
% python ndsign.py sign_module_finalize StratixEngineeringRootKey.qky keys/css/StratixEngrCodeSigningKey_signed.module StratixEngrCodeSigningKey.qky

% python ndsign.py create_root_key --hash-sel=3 --keyfile=keys/css/StratixManufacturingRootKey_Release_pubkey.bin StratixManufacturingRootKey.qky
% python ndsign.py sign_module_finalize StratixManufacturingRootKey.qky keys/css/StratixManufCodeSigningKey_signed.module StratixManufCodeSigningKey.qky


Re-use the same projects to create the engineering and manufacturing root keys.  

% python ndsign.py create_root_key --multi=1 --hash-sel=2 --keyfile=keys/css/StratixEngineeringRootKey_Release_pubkey.bin StratixEngineeringRootKey_fm.qky
% python ndsign.py create_root_key --multi=1 --hash-sel=3 --keyfile=keys/css/StratixManufacturingRootKey_Release_pubkey.bin StratixManufacturingRootKey_fm.qky

Things to check: All Intel root keys should have permissions = 0xFFFFFFFF, cancellation = -1, contribution = 0xFFFFFFFF and enablement = 0.

Sending out sign requests for FM engineering and manufacturing code signing key.

% python ndsign.py create_pubkey_module --multi=1 --permissions=0x1 --cancel_id=0xffffffff keys/css/StratixEngrCodeSigningKey_Debug_pubkey.bin StratixEngrCodeSigningKey_fm.module
% python ndsign.py create_pubkey_module --multi=1 --permissions=0x1 --cancel_id=0xffffffff keys/css/StratixManufCodeSigningKey_Debug_pubkey.bin StratixManufCodeSigningKey_fm.module

On windows

> winpty python ndsign.py --debug sign_module_request --multi=1 --css-project=StratixManufacturingRootKey keys/css/StratixManufacturingRootKey_fm.qky StratixManufCodeSigningKey_fm.module
> winpty python ndsign.py --debug sign_module_approve --multi=1 --approval_type reviewer StratixManufCodeSigningKey_fm.module
> winpty python ndsign.py --debug sign_module_finalize --multi=1 keys/css/StratixManufacturingRootKey_fm.qky StratixManufCodeSigningKey_fm.module StratixManufCodeSigningKey_fm.qky

> winpty python ndsign.py --debug sign_module_request --multi=1 --css-project=StratixEngineeringRootKey keys/css/StratixEngineeringRootKey_fm.qky StratixEngrCodeSigningKey_fm.module
> winpty python ndsign.py --debug sign_module_approve --multi=1 --approval_type reviewer StratixEngrCodeSigningKey_fm.module
> winpty python ndsign.py --debug sign_module_finalize --multi=1 keys/css/StratixEngineeringRootKey_fm.qky StratixEngrCodeSigningKey_fm.module StratixEngrCodeSigningKey_fm.qky


============================
Creating ND codesign key #2
============================
python ndsign.py create_pubkey_module --permissions=0x1 --cancel_id=0x2 keys/css/Stratix10CodeSigningKey2_Release_pubkey.bin Stratix10CodeSigningKey2.module

On windows
> make production-mode
> winpty python ndsign.py --debug sign_module_request --css-project=STRATIX10ROOTKEY keys/css/STRATIX10ROOTKEY.qky Stratix10CodeSigningKey2.module
> winpty python ndsign.py --debug sign_module_finalize keys/css/STRATIX10ROOTKEY.qky Stratix10CodeSigningKey2.module Stratix10CodeSigningKey2.qky

============================
Creating ND codesign key #4
============================
python ndsign.py create_pubkey_module --permissions=0x1 --cancel_id=0x4 keys/css/Stratix10CodeSigningKey4_Release_pubkey.bin Stratix10CodeSigningKey4.module

On windows
> make production-mode
> winpty python ndsign.py --debug sign_module_request --css-project=STRATIX10ROOTKEY keys/css/STRATIX10ROOTKEY.qky Stratix10CodeSigningKey4.module
> winpty python ndsign.py --debug sign_module_finalize keys/css/STRATIX10ROOTKEY.qky Stratix10CodeSigningKey4.module Stratix10CodeSigningKey4.qky

==============================
Falconmesa Keys for RevA
==============================
Falconmesa RevA has a bootrom bug HSD-1707012663 that only supports hash selector 0 and 1.  
RevB and beyond will support 0 ~ 3.
To work around this bug, a hand modified file was created, based on StratixManufCodeSigningKey_fm.qky.
Address 0x14 of this file was changed from 2 to 0 to force the Intel key.  When RevB is released, the 
original StratixManufCodeSigningKey_fm.qky can be used.


