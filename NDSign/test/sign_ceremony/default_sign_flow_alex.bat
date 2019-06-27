@REM This script will walk you though the basic steps to sign a key
@REM Run this script with: ./default_sign_flow_alex.bat

@REM ---------------------------------------------------------------------------
@echo Step 0: Setup you environment. Add ndsign to PATH.
@setlocal enableDelayedExpansion
@REM This script assumes you are configured to use the test css server
@set THIS_DIR=%~dp0
@set NDSIGN_ROOT=%THIS_DIR%..\..
@set PATH=%NDSIGN_ROOT%;%PATH%
@set NDSIGN_INTERACTIVE=winpty python %NDSIGN_ROOT%\ndsign.py

@REM Need to set this because I'm reviewer 10-12. Refer to sign/css.py for more details. 
@set REVIEWER2=reviewer11
@set REVIEWER3=reviewer12

@REM where ndsign
@REM ndsign --help

@del *.qky

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 1: Create root.qky file (Master of Sign Ceremony)
@REM call ndsign create_root_key --help

call %NDSIGN_INTERACTIVE% create_root_key --keyfile=%NDSIGN_ROOT%\keys\css\PSGTest2_Release_pubkey.bin PSGTest2_root.qky

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 2: Request that a key be signed (Master of Sign Ceremony)
@REM call ndsign sign_key_request --help

call %NDSIGN_INTERACTIVE% sign_key_request --permissions=0x1 --cancel_id=0x0 --css-project=PSGTest2 --keyfile=%NDSIGN_ROOT%\keys\css\PSGTest1_Debug_pubkey.bin PSGTest2_root.qky PSGTest1_codesign1_unsigned.qky

@echo If successful, note the XX Request ID in the tag [RequestID:XX]. You will be prompted for this Request ID in the next 5 steps, so remember it. A Request ID equal to 1 probably means something went wrong.

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 3: Key Approve (Reviewer)
@REM call ndsign sign_key_approve --help

call %NDSIGN_INTERACTIVE% sign_key_approve --approval_type reviewer PSGTest1_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 4: Key Approve (Reviewer11)

@REM Note that "--debug" and "reviewer2" are required here only if reviewer1 and reviewer2 
@REM are the same user. This should not happen normally and reveiwer2 approval should be 
@REM identical to Step 3 for all reviewers

call %NDSIGN_INTERACTIVE% --debug sign_key_approve --approval_type reviewer11 PSGTest1_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 5: Key Approve (Reviewer12)

@REM Just like in Step 4...
@REM Note that "--debug" and "reviewer3" are required here only if reviewer1 and reviewer3 
@REM are the same user. This should not happen normally and reveiwer3 approval should be 
@REM identical to Step 3 for all reviewers

call %NDSIGN_INTERACTIVE% --debug sign_key_approve --approval_type reviewer12 PSGTest1_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 6: Key Approve (Manager)

call %NDSIGN_INTERACTIVE% sign_key_approve --approval_type manager PSGTest1_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 7: Key Approve (Security)

call %NDSIGN_INTERACTIVE% sign_key_approve --approval_type security PSGTest1_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 8: Key Finalize (Master of Sign Ceremony)
@REM call ndsign sign_key_finalize --help

call %NDSIGN_INTERACTIVE% sign_key_finalize PSGTest1_codesign1_unsigned.qky PSGTest1_codesign1_signed.qky
@REM ---------------------------------------------------------------------------


@echo Type this to view the qky file: 
@echo ..\..\ndsign objdump PSGTest1_codesign1_signed.qky

@echo
@echo This concludes the Sign Ceremony. Thank you for coming.

@endlocal