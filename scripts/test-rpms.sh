#! /bin/bash
#
yum install -y *.rpm
fpgainfo -h
fpgaconf -h
pci_device -h
fpgasupdate -h
hssi -h
dummy_afu -h
opae.io  -h
PACSign -h
fpgadiag -h

dd if=/dev/urandom  of=dummy.bin bs=1 count=1024
PACSign PR -H openssl_manager -t UPDATE -y -i dummy.bin -o dummy.signed






