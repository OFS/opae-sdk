@REM This script will walk you though the basic steps to sign a modules
@REM This script is tailored to Alex's account because it signs everything with PSGTest2 and assumes reviewer4/5. 

@REM ---------------------------------------------------------------------------
@echo Step 0: Setup you environment. Add ndsign to PATH.
@setlocal enableDelayedExpansion
@REM This script assumes you are configured to use the test css server
@set THIS_DIR=%~dp0
@set NDSIGN_ROOT=%THIS_DIR%..\..
@set PATH=%NDSIGN_ROOT%;%PATH%
@set NDSIGN_INTERACTIVE=winpty python %NDSIGN_ROOT%\ndsign.py

@REM Need to set this because I'm reviewer 3-5. Refer to sign/css.py for more details. 
@set REVIEWER2=reviewer4
@set REVIEWER3=reviewer5

@REM where ndsign
@REM ndsign --help

@del *.qky
@del *.module

@set UNSIGNED_MODULE_FILE=PSGTest1_unsigned.module

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 1: Create root.qky file (Master of Sign Ceremony)
@REM call ndsign create_root_key --help

call %NDSIGN_INTERACTIVE% create_root_key --keyfile=%NDSIGN_ROOT%\keys\css\PSGTest2_Release_pubkey.bin PSGTest2_root.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 2: Create public key module file PSGTest1_unsigned.module (Master of Sign Ceremony)
@REM call ndsign create_root_key --help

call %NDSIGN_INTERACTIVE% create_pubkey_module %NDSIGN_ROOT%\keys\css\PSGTest1_Debug_pubkey.bin %UNSIGNED_MODULE_FILE%

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 2: Request that a module be signed (Master of Sign Ceremony)
@REM call ndsign sign_module_request --help

call %NDSIGN_INTERACTIVE% sign_module_request --permissions=0x1 --cancel_id=0x0 --css-project=PSGTest2 %NDSIGN_ROOT%\keys\css\PSGTest2.qky %UNSIGNED_MODULE_FILE%

@echo If successful, note the XX Request ID in the tag [RequestID:XX]. You will be prompted for this Request ID in the next 5 steps, so remember it. A Request ID equal to 1 probably means something went wrong.

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 3: Module Approve (Reviewer)
@REM call ndsign sign_module_approve --help

call %NDSIGN_INTERACTIVE% sign_module_approve --approval_type reviewer %UNSIGNED_MODULE_FILE%

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 4: Module Approve (%REVIEWER2%)

@REM Note that "--debug" and "reviewer2" are required here only if reviewer1 and reviewer2 
@REM are the same user. This should not happen normally and reveiwer2 approval should be 
@REM identical to Step 3 for all reviewers

call %NDSIGN_INTERACTIVE% --debug sign_module_approve --approval_type %REVIEWER2% %UNSIGNED_MODULE_FILE%

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 5: Module Approve (%REVIEWER3%)

@REM Just like in Step 4...
@REM Note that "--debug" and "reviewer3" are required here only if reviewer1 and reviewer3 
@REM are the same user. This should not happen normally and reveiwer3 approval should be 
@REM identical to Step 3 for all reviewers

call %NDSIGN_INTERACTIVE% --debug sign_module_approve --approval_type %REVIEWER3% %UNSIGNED_MODULE_FILE%

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 6: Module Approve (Manager)

call %NDSIGN_INTERACTIVE% sign_module_approve --approval_type manager %UNSIGNED_MODULE_FILE%

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 7: Module Approve (Security)

call %NDSIGN_INTERACTIVE% sign_module_approve --approval_type security %UNSIGNED_MODULE_FILE%

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 8: Module Finalize (Master of Sign Ceremony)
@REM call ndsign sign_module_finalize --help

call %NDSIGN_INTERACTIVE% sign_module_finalize PSGTest2_root.qky %UNSIGNED_MODULE_FILE% PSGTest1_codesign1_signed.qky
@REM ---------------------------------------------------------------------------


@echo Type this to view the qky file: 
@echo ..\..\ndsign objdump PSGTest1_codesign1_signed.qky

@echo ""
@echo This concludes the Sign Ceremony. Thank you for coming.

@endlocal