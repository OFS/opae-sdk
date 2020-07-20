# PACSign #

## SYNOPSIS ##
`python PACSign.py [-h] {FIM,SR,BBS,BMC,BMC_FW,AFU,PR,GBS} ...`

`python PACSign.py <CMD>  [-h] -t {UPDATE,CANCEL,RK_256,RK_384} -H HSM_MANAGER
                          [-C HSM_CONFIG] [-r ROOT_KEY] [-k CODE_SIGNING_KEY]
                          [-d CSK_ID] [-i INPUT_FILE] [-o OUTPUT_FILE] [-y] [-v]`

## DESCRIPTION ##
The `PACSign` utility inserts authentication markers into bitstreams targeted for the following programmable
acceleration cards (PACs):
* Intel&reg; PAC with Intel Arria&reg; 10 GX FPGA
* Intel FPGA PAC D5005
* Intel PAC N3000
`PACSign` uses a root key and an optional code signing key to digitally sign the  
bitstreams to validate their origin. The PACs will not program bitstreams without proper authentication.

The current PACs only support elliptical curve keys with the curve type `secp256r1` or `prime256v1`.
The `PACSign` command supports hardware security modules (HSMs) for both `OpenSSL` and `PKCS #11`.

To utilize `PKCS #11`, please ensure that the dummy fields `lib_path`,
`token.label`, and `token.user_password` in the configuration file
`PKCS11_config.json` are modified appropriately before proceeding.

## BITSTREAM TYPES ##
The first required argument to `PACSign` is the bitstream type identifier.

`{SR,FIM,BBS,BMC,BMC_FW,PR,AFU,GBS}`

Supported image types. `FIM` and `BBS` are aliases for the static region (`SR`). `BMC_FW` is an alias for 
the board management controller (`BMC`). `AFU` and `GBS` are aliases for the partial reconfiguration (`PR`) region.

 `SR (FIM, BBS)`
 
 Static FPGA image
 
 `BMC (BMC_FW)`
 
 BMC image, including firmware for some PACs
 
 `PR (AFU, GBS)`
 
 Reconfigurable FPGA image


## REQUIRED OPTIONS ##

All bitstream types must specify the following information:
* An operation that the `PACSign` command performs
* The name and optional parameter file for a key signing module

`-t, --cert_type <type>`

The following operations are available: 

    `UPDATE` - add authentication data to the bitstream.
    `CANCEL` - create a code signing key cancellation bitstream.
    `RK_256` - create a bitstream to program a 256-bit root entry hash into the device.
    `RK_384` - create a bitstream to program a 384-bit root entry hash into the device. Currently 
               available PACs do not support a 384-bit root entry hash.
    

`-H, --HSM_manager <module>`

The module name for a module that interfaces to a HSM.  `PACSign`
includes both the `openssl_manager` and the `pkcs11_manager` to handle keys and signing
operations.

`-C, --HSM_config <cfg>` (optional)

The argument to this option is passed verbatim to the specified HSM.
For `pkcs11_manager`, this option specifies a `JSON` file describing the PKCS #11
capable HSM's parameters.

## OPTIONS ##

`-r, --root_key <keyID>`

The key identifier that the HSM uses to identify the root key
to be used for the selected operation.

`-k, --code_signing_key <keyID>`

The key identifier that the HSM uses to identify the code
signing key to be used for the selected operation.

`-d, --csk_id <csk_num>`

Only used for type `CANCEL`. Specifies the key number of the code signing key to
cancel.

`-i, --input_file <file>`

Only used for `UPDATE` operations. Specifies the file name containing the data
to be signed.

`-o, --output_file <file>`

Specifies the file name for the signed output bitstream.

`-y, --yes`

Silently answer all queries from `PACSign` in the affirmative.

`-v, --verbose`

Can be specified multiple times.  Increases the verbosity of `PACSign`. Once
enables non-fatal warnings to be displayed. Twice enables progress information.
Three or more occurrences enables very verbose debugging information.

## NOTES ##

Different certification types require different sets of options.  The table below
describes the options required based on certification type:

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

[^2]: For `UPDATE` type, you must specify both keys to produce an authenticated bitstream.
Omitting one key generates a valid, but unauthenticated bitstream. You can only load the
unauthenticated bitstream on a PAC with no root entry hash programmed for that bitstream type.

## EXAMPLES ##

The following command generates a root hash programming PR bitstream.
The generated file can be an input to the `fpgasupdate` command to program the root hash for
PR operations into the device flash.  You can only program the root hash one time. 

`python PACSign.py PR -t RK_256 -o pr_rhp.bin -H openssl_manager -r key_pr_root_public_256.pem`

The following command adds authentication blocks to `hello_afu.gbs` signed
by both provided keys and writes the result to `s_hello_afu.gbs`. For signed bitstreams, 
the newly-generated block replaces the old signature block. 

`python PACSign.py PR -t update -H openssl_manager -i hello_afu.gbs -o s_hello_afu.gbs -r key_pr_root_public_256.pem -k key_pr_csk0_public_256.pem`

The following command generates a code signing key cancellation bitstream
to cancel code signing key number 4 for all BMC operations. After this command runs the PAC rejects BMC
images that CSK 4 signed.

`python PACSign.py BMC -t cancel -H openssl_manager -o csk4_cancel.gbs -r key_bmc_root_public_256.pem -d 4`

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2019.07.08 | 1.2.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 19.2.) | Initial revision.  | 
 | 2019.10.04 | 2.0.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 19.2.) | Editorial changes only. |
