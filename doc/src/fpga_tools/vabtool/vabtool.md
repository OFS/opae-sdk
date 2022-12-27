# vabtool #

## SYNOPSIS ##

`vabtool [-r RETRIES] [-d] [-y] [-v] <ACTION>`

Where ACTION is defined as one of the following:

`vabtool sr_key_provision PCIE_ADDRESS SR_RKH_FILE FPGA_IMG_FILE`<br>
`vabtool sr_status PCIE_ADDRESS`<br>
`vabtool pr_key_provision PCIE_ADDRESS PR_AUTH_CERT_FILE PR_RKH_FILE`<br>
`vabtool pr_status PCIE_ADDRESS`<br>
`vabtool sr_key_cancel PCIE_ADDRESS SR_RKH_CANCEL_FILE`<br>
`vabtool sr_cancel_status PCIE_ADDRESS`<br>
`vabtool pr_key_cancel PCIE_ADDRESS PR_RKH_CANCEL_FILE`<br>
`vabtool pr_cancel_status PCIE_ADDRESS`

## DESCRIPTION ##

The ```vabtool``` command helps perform Vendor Authenticated Boot
provisioning of Static Region and Partial Reconfiguration Region key
hashes and helps perform SR and PR hash cancellation and status reporting.

## OPTIONS ##

`-r RETRIES, --retries RETRIES`

    Specifies the number of times a failed SR or PR key provision is to be
    retried. The default value for RETRIES is 3.

`-d, --dry-run`

    Don't execute the actual fpgasupdate and rsu commands, but only print
    the commands that would be executed during a normal run of the script.

`-y, --yes`

    The tool will respond with an automatic Yes answer to all confirmation
    prompts posed by the sub-tools.

`-v, --version`

    Display script version information and exit.

## EXAMPLES ##

`sudo vabtool -y sr_key_provision 0000:bc:00.0 my_sr_rkh.bin my_fpga.bin`<br>
`sudo vabtool sr_status 0000:bc:00.0`<br>
`sudo vabtool -y pr_key_provision 0000:bc:00.0 pr_auth_cert.bin my_pr_rkh.bin`<br>
`sudo vabtool pr_status 0000:bc:00.0`<br>
`sudo vabtool sr_key_cancel 0000:bc:00.0 my_sr_rhk_cancel.bin`<br>
`sudo vabtool sr_cancel_status 0000:bc:00.0`<br>
`sudo vabtool pr_key_cancel 0000:bc:00.0 my_pr_rhk_cancel.bin`<br>
`sudo vabtool pr_cancel_status 0000:bc:00.0`

## Revision History ##

Document Version | Intel Acceleration Stack Version | Changes
-----------------|----------------------------------|--------
2022.10.31 | IOFS 2022.4 | Initial release.
