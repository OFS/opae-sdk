# PACSign #

## SYNOPSIS ##
`python PACSign.py [-h] {FIM,SR,BBS,BMC,BMC_FW,AFU,PR,GBS} ...`

`python PACSign.py <CMD>  [-h] -t {UPDATE,CANCEL,RK_256,RK_384} -H HSM_MANAGER
                          [-C HSM_CONFIG] [-r ROOT_KEY] [-k CODE_SIGNING_KEY]
                          [-d CSK_ID] [-i INPUT_FILE] [-o OUTPUT_FILE] [-y] [-v]`

## DESCRIPTION ##
`PACSign` is a utility designed to insert proper authentication markers on bitstreams targeted for the PACs. To accomplish this,
it uses a root key and an optional code signing key to digitally sign the bitstreams to validate their origin.
The PACs will not accept loading bitstreams without proper authentication.

The current PACs only support elliptical curve keys with the curve type `secp256r1` or `prime256v1`.
PACSign is distributed with managers for both `OpenSSL` and `PKCS #11`.

## BITSTREAM TYPES ##
The first required argument to `PACSign` is the bitstream type identifier.

`{SR,FIM,BBS,BMC,BMC_FW,PR,AFU,GBS}`

Allowable image types.  `FIM` and `BBS` are aliases for `SR`, `BMC_FW` is an alias for `BMC`, and
`AFU` and `GBS` are aliases for `PR`.

 `SR (FIM, BBS)`
 
 Static FPGA image
 
 `BMC (BMC_FW)`
 
 BMC image, including firmware for some PACs
 
 `PR (AFU, GBS)`
 
 Reconfigurable FPGA image


## REQUIRED OPTIONS ##

All bitstream types are required to include an action to be performed by `PACSign`
and the name and optional parameter file for a key signing module.

`-t, --cert_type <type>`

Values must be one of `UPDATE`, `CANCEL`, `RK_256`, or `RK_384`[^1].

    `UPDATE` - add authentication data to the bitstream.
    `CANCEL` - create a code signing key cancellation bitstream.
    `RK_256` - create a bitstream to program a 256-bit root key to the device.
    `RK_384` - create a bitstream to program a 384-bit root key to the device.
    
[^1]:Current PACs do not support 384-bit root keys.

`-H, --HSM_manager <module>`

The module name for a manager that is used to interface to an HSM.  `PACSign`
supplies both `openssl_manager` and `pkcs11_manager` to handle keys and signing
operations.

`-C, --HSM_config <cfg>` (optional)

The argument to this option is passed verbatim to the specified HSM manager.
For `pkcs11_manager`, this option specifies a `JSON` file describing the PKCS #11
capable HSM's parameters.

## OPTIONS ##

`-r, --root_key <keyID>`

The key identifier recognizable to the HSM manager that identifies the root key
to be used for the selected operation.

`-k, --code_signing_key <keyID>`

The key identifier recognizable to the HSM manager that identifies the code
signing key to be used for the selected operation.

`-d, --csk_id <csk_num>`

Only used for type `CANCEL` and is the key number of the code signing key to
cancel.

`-i, --input_file <file>`

Only used for `UPDATE` operations. Specifies the file name containing the data
to be signed.

`-o, --output_file <file>`

Specifies the name of the file to which the signed bitstream is to be written.

`-y, --yes`

Silently answer all queries from `PACSign` in the affirmative.

`-v, --verbose`

Can be specified multiple times.  Increases the verbosity of `PACSign`. Once
enables non-fatal warnings to be displayed; twice enables progress information.
Three or more occurrences enables very verbose debugging information.

## NOTES ##

Different certification types require different sets of options.  The table below
describes which options are required based on certification type:

### UPDATE ###

| | root_key | code_signing_key | csk_id | input_file | output_file |
|---|---|---|---|---|---|
| SR | Optional[^2] | Optional[^2] | No | Yes | Yes |
| BMC | Optional[^2] | Optional[^2] | No | Yes | Yes |
| PR | Optional[^2] | Optional[^2] | No | Yes | Yes |

### CANCEL ###

| | root_key | code_signing_key | csk_id | input_file | output_file |
|---|---|---|---|---|---|
| SR | Yes | No | Yes | No | Yes |
| BMC | Yes | No | Yes | No | Yes |
| PR | Yes | No | Yes | No | Yes |

### RK_256 / RK_384[^1] ###

| | root_key | code_signing_key | csk_id | input_file | output_file |
|---|---|---|---|---|---|
| SR | Yes | No | No | No | Yes |
| BMC | Yes | No | No | No | Yes |
| PR | Yes | No | No | No | Yes |

[^2]: For UPDATE type, both keys must be specified to produce an authenticated bitstream.
Omitting one key generates a valid, but unauthenticated bitstream that can only be loaded
on a PAC with no root key programmed for that type.

## EXAMPLES ##

The following command will generate a root hash programming PR bitstream.
The generated file can be given to `fpgasupdate` to program the root hash for
PR operations into the device flash.  Note that root hash programming can only
be done once on a PAC.

`python PACSign.py PR -t RK_256 -o pr_rhp.bin -H openssl_manager -r key_pr_root_public_256.pem`

The following command will add authentication blocks to `hello_afu.gbs` signed
by both provided keys and write the result to `s_hello_afu.gbs`.  If the input
bitstream were already signed, the old signature block is replaced with the
newly-generated block.

`python PACSign.py PR -t update -H openssl_manager -i hello_afu.gbs -o s_hello_afu.gbs -r key_pr_root_public_256.pem -k key_pr_csk0_public_256.pem`

The following command will generate a code signing key cancellation bitstream
to cancel code signing key 4 for all BMC operations. CSK 4 bitstreams that attempt
to load BMC images will be rejected by the PAC.

`python PACSign.py BMC -t cancel -H openssl_manager -o csk4_cancel.gbs -r key_bmc_root_public_256.pem -d 4`

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2019.07.08 | 1.2.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 19.2.) | Initial revision.  | 
