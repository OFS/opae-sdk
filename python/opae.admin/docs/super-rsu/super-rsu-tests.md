# test cases #


## scenarios ##

1. Basic install/remove
   Install from RPM X number of times and verify all device features are
   accessible. This will flash the following images:
   * user
   * bmc_img
   * bmc_fw

   _Note_: If the versions of the components being flashed on the system
   under test match the versions in the RPM, those components will not be
   flashed. In order to ensure that these are flashed, a custom RPM will
   have to be generated with the manifest including { 'force' : true } in
   the flash spec for each component being flashed. The following lists
   what the three flash specs should look like for testing on a system
   with a rev C board.

```JSON
{
    "product": "n3000",
    "vendor": "0x8086",
    "device": "0x0b30",
    "additional_devices": [["0x8086", "0x0b32"]],
    "flash" : [
         {
             "filename": "vista_creek_qspi_xip_v1.0.6.ihex",
             "type": "bmc_fw",
             "version": "1.0.6",
	     "force": true
         },
         {
	     "filename": "max10_system_revc_dual_v1.0.6_cfm0_auto.rpd",
             "type": "bmc_img",
             "version": "1.0.6",
             "revision": "c",
	     "force": true
         },
         {
             "filename": "vcp_a10_flash_ww11.3_rc4_2x1x25.bin",
             "type": "user",
             "version": "2.0.0",
	     "force": true
         }
    ]
}
```

   To create the custom RPM, the following must be done:
   1. clone OPAE/super-rsu repository
   2. Modify the flash specs for the three image types in rsu-n300.json
      Use the example above for the content of the manifest.
   3. Create the super rsu RPM.
      1. Call create-rpm bash script.
      2. The RPM will be available in ~/rpmbuild/RPMS/noarch



   ### test steps ###
   1. Install from opae-super-rsu.rpm
      This should install dependencies:
      * opae-intel-fpga-driver
      * opae-tools-extra
   2. Verify super rsu files (firmware and manifest) installed in
      /usr/share/opae/n3000
   3. Run "fpgainfo bmc" (dependency on opae-tools)
      1. Verify reported versions are what is expected
      2. Verify we can see sensor readings
   4. Uninstall opae-super-rsu, opae-intel-fpga-driver, opae-tools-extra rpms
      1. Verify super rsu files (firmware and manifest) are removed from
         /usr/share/opae/n3000
      2. Verify no fpga sysfs nodes exist under /sys/class/fpga
   5. Repeat 1-4 100 times

2. Install on a system that is up to date.
   Installing a super-rsu RPM on a system that has the components being flashed
   as the same versions of the images listed in the manifest should never have
   fpgaflash called. To verify this, one may see the logs printed to standard
   out by calling yum install with the verbose flag (-v). The output should look
   like the following:

   ```console
   [2019-03-25 08:38:17,998] [INFO    ] [db:00.0     ] - nothing to do for bmc_fw
   image, already up to date
   [2019-03-25 08:38:17,998] [INFO    ] [3e:00.0     ] - nothing to do for bmc_fw
   image, already up to date
   [2019-03-25 08:38:17,999] [INFO    ] [db:00.0     ] - nothing to do for bmc_img
   image, already up to date
   [2019-03-25 08:38:17,999] [INFO    ] [3e:00.0     ] - nothing to do for bmc_img
   image, already up to date
   [2019-03-25 08:38:17,999] [INFO    ] [db:00.0     ] - nothing to do for user
   image, already up to date
   [2019-03-25 08:38:17,999] [INFO    ] [3e:00.0     ] - nothing to do for user
   image, already up to date
   ```

   _Note_: The preceding example shows output from a system with two VC cards,
   one on bus db, and the other on bus 3e.

3. Interrupt an RPM installation
   Install a super-rsu RPM and hit Control-C on the keyboard to interrupt it.
   Ensure that no files are installed in /usr/share/opae/n3000. Because the
   process may be interrupted while fpgaflash is writing to the flash device,
   it is possible to leave the system in an indeterminate state. Because the
   images being flashed are user images, the system should be recoverable by
   issuing rsu command to the board.
   To reset the user components on the board, invoke super-rsu script with
   --rsu-only command line flag. Once the board is reset, ensure device
   features are accessible.


