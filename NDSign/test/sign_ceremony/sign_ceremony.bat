@REM This script will walk you though the sign key ceremony steps

@REM ---------------------------------------------------------------------------
@echo Step 0: Setup you environment. Add ndsign to PATH.
@setlocal enableDelayedExpansion
@REM This script assumes you are configured to use the test css server
@set THIS_DIR=%~dp0
@set NDSIGN_ROOT=%THIS_DIR%..\..
@set PATH=%NDSIGN_ROOT%;%PATH%

@REM where ndsign
@REM ndsign --help

@del *.qky

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 1: Create root.qky file (Master of Sign Ceremony)
@REM call ndsign create_root_key --help

call ndsign create_root_key --keyfile=%NDSIGN_ROOT%\keys\css\PSGTest1_Release_pubkey.bin PSGTest1_root.qky

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 2: Request that a key be signed (Master of Sign Ceremony)
@REM call ndsign sign_key_request --help

call ndsign sign_key_request --permissions=0x1 --cancel_id=0x0 --css-project=PSGTest1 --keyfile=%NDSIGN_ROOT%\keys\css\PSGTest2_Debug_pubkey.bin PSGTest1_root.qky PSGTest2_codesign1_unsigned.qky

@echo If successful, note the XX Request ID in the tag [RequestID:XX]. You will be prompted for this Request ID in the next 5 steps, so remember it. A Request ID equal to 1 probably means something went wrong.

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 3: Key Approve (Reviewer1)
@REM call ndsign sign_key_approve --help

call ndsign sign_key_approve --approval_type reviewer PSGTest2_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 4: Key Approve (Reviewer2)

@REM Note that "--debug" and "reviewer2" are required here only if reviewer1 and reviewer2 
@REM are the same user. This should not happen normally and reveiwer2 approval should be 
@REM identical to Step 3 for all reviewers

call ndsign --debug sign_key_approve --approval_type reviewer2 PSGTest2_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 5: Key Approve (Reviewer3)

@REM Just like in Step 4...
@REM Note that "--debug" and "reviewer3" are required here only if reviewer1 and reviewer3 
@REM are the same user. This should not happen normally and reveiwer3 approval should be 
@REM identical to Step 3 for all reviewers

call ndsign --debug sign_key_approve --approval_type reviewer3 PSGTest2_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 6: Key Approve (Manager)

call ndsign sign_key_approve --approval_type manager PSGTest2_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------

@REM ---------------------------------------------------------------------------
@echo Step 7: Key Approve (Security)

call ndsign sign_key_approve --approval_type security PSGTest2_codesign1_unsigned.qky

@REM ---------------------------------------------------------------------------


@REM ---------------------------------------------------------------------------
@echo Step 8: Key Finalize (Master of Sign Ceremony)
@REM call ndsign sign_key_finalize --help

call ndsign sign_key_finalize  PSGTest2_codesign1_unsigned.qky PSGTest2_codesign1_signed.qky
@REM ---------------------------------------------------------------------------


@echo Type this to view the qky file: 
@echo ..\..\ndsign objdump PSGTest2_codesign1_signed.qky

@echo
@echo This concludes the Sign Ceremony. Thank you for coming.

@endlocal