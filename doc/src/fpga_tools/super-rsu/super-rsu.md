# Super Remote System Update User Guide #

.. toctree::

.. highlight:: c

.. highlight:: console

## Overview ##
Intel Programmable Acceleration Card (PAC) devices are comprised of multiple processors
and controllers that execute firmware. Maintaining and updating these firmware images
manually is error-prone and does not scale well within the Data Center. The solution
described here is derived with the following goals in mind:

* The ability to update one or more (possibly all) firwmare images with a single package.
* The ability to complete all firmware updates within a stipulated time window.
* The ability to update each PAC in the server, all servers in a Data Center, and multiple
Data Centers remotely.
* The ability to remotely initiate download of the package and its installation with a
single command per server instance.
* The ability to roll back firmware to a previous revision.

## Implementation ##
A single package containing firmware images for all programmable parts on a PAC is delivered
as an RPM, eg opae-super-rsu-n3000.M.m.p-r.noarch.rpm. The RPM revision will sequentially increase
with every update.

Installing or upgrading the RPM invokes the complete update of all programmable parts on all
PAC boards in the system.

The standard RPM dependency framework ensures that correct versions of dependecy packages
opae-intel-fpga-driver and fpga-tools-extra are installed on the system.

Rolling back is achieved by uninstalling the current version and re-installing a previous
version of the RPM.

.. note::
```
Note: once Secure Update is deployed, roll back restrictions shall be implemented to prevent
rollback attacks.
```

RPM management on remote systems is standard practice, requiring no new infrastructure/training.

## Details ##

The post-install hook of the opae-super-rsu-n3000 RPM is leveraged to call out to the super-rsu
Python script to update all PAC boards. super-rsu uses the manifest file packaged within
opae-super-rsu-n3000 to associate a firmware image with its version. Each of the firmware images
contained in opae-super-rsu-n3000 is placed on the target system in /usr/share/opae/n3000.

### Algorithm ###

* Acquire the current firmware versions of all programmable parts.
* For each programmable image, if the installed version of firmware does not equal the version
provided in the RPM manifest file, then update the firmware image, and set image_updated to True.
* After all updates, if image_updated, then initiate a safe reboot of all boards in the system.
* After safe reboot, verify that the reported firmware versions match those of the RPM manifest.
If they do not match, then RPM installation exits with a failing status.
* Run board self test. If the self test fails, then the RPM installation exits with a failing status.
* If all of the above checks is successful, then RPM installation exits with a success status.

## Dependencies ##

* The standard Python package for the distro (version 2.7).
* The opae-intel-fpga-driver RPM. (version determined by opae-super-rsu-n3000)
* The opae-tools-extra RPM. (version determined by opae-super-rsu-n3000)
