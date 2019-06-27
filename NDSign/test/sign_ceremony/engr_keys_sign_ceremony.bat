@REM This scripts contain the steps to use during the signing ceremony for engineering keys.

@REM ---------------------------------------------------------------------------
@echo Step 0: Setup you environment. Add ndsign to PATH.
@setlocal enableDelayedExpansion
@REM This script assumes you are configured to use the test css server
@set THIS_DIR=%~dp0
@set NDSIGN_ROOT=%THIS_DIR%..\..
@set PATH=%NDSIGN_ROOT%;%PATH%
@set NDSIGN_INTERACTIVE=winpty python %NDSIGN_ROOT%\ndsign.py

@REM where ndsign
@REM ndsign --help

@REM Remember to switch to development server before executing this script 
@REM make production-mode

@del *.qky
@del *.module

@REM ---------------------------------------------------------------------------
@echo Step 1: Create public key module file StratixEngrCodeSigningKey_unsigned.module (Master of Sign Ceremony)
@REM call ndsign create_pubkey_module --help

call winpty python ndsign.py create_pubkey_module --permission=0x1 --cancel_id=0xffffffff keys/css/StratixEngrCodeSigningKey_Debug_pubkey.bin StratixEngrCodeSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 2: Request that a module be signed (Master of Sign Ceremony)
@REM call ndsign sign_module_request --help

call winpty python ndsign.py sign_module_request --permissions=0x1 --cancel_id=0xffffffff --css-project=StratixEngineeringRootKey keys/css/StratixEngineeringRootKey.qky StratixEngrCodeSigningKey_unsigned.module

@echo If successful, note the XX Request ID in the tag [RequestID:XX]. You will be prompted for this Request ID in the next 5 steps, so remember it. A Request ID equal to 1 probably means something went wrong.

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 3: Module Approve (Reviewer)
@REM call ndsign sign_module_approve --help

call winpty python ndsign.py sign_module_approve --approval_type reviewer StratixEngrCodeSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------
@echo Step 6: Module Approve (Manager)

call winpty python ndsign.py sign_module_approve --approval_type manager StratixEngrCodeSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------

@echo Step 7: Module Approve (Security)

call winpty python ndsign.py sign_module_approve --approval_type security StratixEngrCodeSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 8: Module Finalize (Master of Sign Ceremony)
@REM call ndsign sign_module_finalize --help

call winpty python ndsign.py sign_module_finalize keys/css/StratixEngineeringRootKey.qky StratixEngrCodeSigningKey_unsigned.module StratixEngrCodeSigningKey.qky
@REM ---------------------------------------------------------------------------


@echo Type this to view the qky file: 
@echo python ndsign.py objdump StratixEngrCodeSigningKey.qky

@echo "---------------------------------------------------------------------------"
@echo "---------------------------------------------------------------------------"
@REM ---------------------------------------------------------------------------
@echo Step 1: Create public key module file Stratix10EngrCertSigningKey_unsigned.module (Master of Sign Ceremony)
@REM call ndsign create_pubkey_module --help

call winpty python ndsign.py create_pubkey_module --permission=0x10000 --cancel_id=31 keys/css/Stratix10EngrCertSigningKey_Release_pubkey.bin Stratix10EngrCertSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 2: Request that a module be signed (Master of Sign Ceremony)
@REM call ndsign sign_module_request --help

call winpty python ndsign.py sign_module_request --permission=0x10000 --cancel_id=31 --css-project=STRATIX10ROOTKEY keys/css/STRATIX10ROOTKEY.qky Stratix10EngrCertSigningKey_unsigned.module

@echo If successful, note the XX Request ID in the tag [RequestID:XX]. You will be prompted for this Request ID in the next 5 steps, so remember it. A Request ID equal to 1 probably means something went wrong.

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 3: Module Approve (Reviewer)
@REM call ndsign sign_module_approve --help

call winpty python ndsign.py sign_module_approve --approval_type reviewer Stratix10EngrCertSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------
@echo Step 6: Module Approve (Manager)

call winpty python ndsign.py sign_module_approve --approval_type manager Stratix10EngrCertSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------

@echo Step 7: Module Approve (Security)

call winpty python ndsign.py sign_module_approve --approval_type security Stratix10EngrCertSigningKey_unsigned.module

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 8: Module Finalize (Master of Sign Ceremony)
@REM call ndsign sign_module_finalize --help

call winpty python ndsign.py sign_module_finalize keys/css/STRATIX10ROOTKEY.qky Stratix10EngrCertSigningKey_unsigned.module Stratix10EngrCertSigningKey.qky
@REM ---------------------------------------------------------------------------

@echo Type this to view the qky file: 
@echo python ndsign.py objdump Stratix10EngrCertSigningKey.qky

@echo ""
@echo This concludes the Sign Ceremony. Thank you for coming.

@endlocal
