#!/usr/bin/env python
# Copyright(c) 2017, Intel Corporation
##
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
##
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
##
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
import json
from sysfs import sysfsinfo
from fpga_common import (
    fpga_command,
    bitmask)


class error_mask(object):
    _name = None
    _bitmasks = []

    def __init__(self, value):
        self.value = value

    def __unicode__(self):
        s = u'0x{:02x}'.format(self.value)
        if self.value:
            for name, bm in self._bitmasks:
                err = bm(self.value)
                if err:
                    s += u'\n  {:20} : 0x{:02X}'.format(name, err)
        return s

    def to_dict(self, include_bdf=False):
        if self._bitmasks:
            data = (dict([(name, bm(self.value))
                          for (name, bm) in self._bitmasks]))
            return data
        else:
            return self.value


class fme_errors(error_mask):
    _name = "fme"
    _file = "fme-errors/errors"
    _bitmasks = [
        ("CvlCdcParErro0", bitmask(17, 19)),
        ("Pcie1CdcParErr", bitmask(12, 16)),
        ("Pcie0CdcParErr", bitmask(7, 11)),
        ("MBPErr", bitmask(6, 6)),
        ("AfuAccessModeErr", bitmask(5, 5)),
        ("IommuParityErr", bitmask(4, 4)),
        ("KtiCdcParityErr", bitmask(2, 3)),
        ("FabricFifoUOflow", bitmask(1, 1)),
        ("FabricErr", bitmask(0, 0)),
    ]


class bbs_errors(error_mask):
    _name = "bbs"
    _file = "bbs_errors"
    _bitmasks = [
        ("InjectedCatastErr", bitmask(11, 11)),
        ("ThermCatastErr", bitmask(10, 10)),
        ("CrcCatastErr", bitmask(9, 9)),
        ("InjectedFatalErr", bitmask(8, 8)),
        ("PciePoisonErr", bitmask(7, 7)),
        ("FabricFatalErr", bitmask(6, 6)),
        ("IommuFatalErr", bitmask(5, 5)),
        ("DramFatalErr", bitmask(4, 4)),
        ("KtiProtoFatalErr", bitmask(3, 3)),
        ("CciFatalErr", bitmask(2, 2)),
        ("TagCchFatalErr", bitmask(1, 1)),
        ("KtiLinkFatalErr", bitmask(0, 0)),
    ]


class port_errors(error_mask):
    _name = "port"
    _file = "errors"
    _bitmasks = [
        ("VfFlrAccessError", bitmask(51, 51)),
        ("Ap6Event", bitmask(50, 50)),
        ("PMRError", bitmask(49, 49)),
        ("PageFault", bitmask(48, 48)),
        ("VgaMemRangeError", bitmask(47, 47)),
        ("LegRangeHighError", bitmask(46, 46)),
        ("LegRangeLowError", bitmask(45, 45)),
        ("GenProtRangeError", bitmask(44, 44)),
        ("L1prMesegError", bitmask(43, 43)),
        ("L1prSmrr2Error", bitmask(42, 42)),
        ("L1prSmrrError", bitmask(41, 41)),
        ("TxReqCounterOverflow", bitmask(40, 40)),
        ("UnexpMMIOResp", bitmask(34, 34)),
        ("TxCh2FifoOverflow", bitmask(33, 33)),
        ("MMIOTimedOut", bitmask(32, 32)),
        ("TxCh1NonZeroSOP", bitmask(24, 24)),
        ("TxCh1IncorrectAddr", bitmask(23, 23)),
        ("TxCh1DataPayloadOverrun", bitmask(22, 22)),
        ("TxCh1InsufficientData", bitmask(21, 21)),
        ("TxCh1Len4NotAligned", bitmask(20, 20)),
        ("TxCh1Len2NotAligned", bitmask(19, 19)),
        ("TxCh1Len3NotSupported", bitmask(18, 18)),
        ("TxCh1InvalidReqEncoding", bitmask(17, 17)),
        ("TxCh1Overflow", bitmask(16, 16)),
        ("MMIOWrWhileRst", bitmask(10, 10)),
        ("MMIORdWhileRst", bitmask(9, 9)),
        ("TxCh0Len4NotAligned", bitmask(4, 4)),
        ("TxCh0Len2NotAligned", bitmask(3, 3)),
        ("TxCh0Len3NotSupported", bitmask(2, 2)),
        ("TxCh0InvalidReqEncoding", bitmask(1, 1)),
        ("TxCh0Overflow", bitmask(0, 0)),
    ]


class pcie0_errors(error_mask):
    _name = "pcie0"
    _file = "pcie0_errors"
    _bitmasks = [
        ("FunctTypeErr", bitmask(63, 63)),
        ("VFNumb", bitmask(62, 62)),
        ("RxPoisonTlpErr", bitmask(9, 9)),
        ("ParityErr", bitmask(8, 8)),
        ("CompTimeOutErr", bitmask(7, 7)),
        ("CompStatErr", bitmask(6, 6)),
        ("CompTagErr", bitmask(5, 5)),
        ("MRLengthErr", bitmask(4, 4)),
        ("MRAddrErr", bitmask(3, 3)),
        ("MWLengthErr", bitmask(2, 2)),
        ("MWAddrErr", bitmask(1, 1)),
        ("FormatTypeErr", bitmask(0, 0)),
    ]


class pcie1_errors(error_mask):
    _name = "pcie1"
    _file = "pcie1_errors"
    _bitmasks = [
        ("RxPoisonTlpErr", bitmask(9, 9)),
        ("ParityErr", bitmask(8, 8)),
        ("CompTimeOutErr", bitmask(7, 7)),
        ("CompStatErr", bitmask(6, 6)),
        ("CompTagErr", bitmask(5, 5)),
        ("MRLengthErr", bitmask(4, 4)),
        ("MRAddrErr", bitmask(3, 3)),
        ("MWLengthErr", bitmask(2, 2)),
        ("MWAddrErr", bitmask(1, 1)),
        ("FormatTypeErr", bitmask(0, 0)),
    ]


class gbs_errors(error_mask):
    _name = "gbs"
    _file = "gbs_errors"
    _bitmasks = [
        ("MBPErr", bitmask(12, 12)),
        ("PowerThreshAP2", bitmask(11, 11)),
        ("PowerThreshAP1", bitmask(10, 10)),
        ("TempThreshAP6", bitmask(9, 9)),
        ("InjectedWarningErr", bitmask(6, 6)),
        ("AfuAccessModeErr", bitmask(5, 5)),
        ("ProcHot", bitmask(4, 4)),
        ("PortFatalErr", bitmask(3, 3)),
        ("PcieError", bitmask(2, 2)),
        ("TempThreshAP2", bitmask(1, 1)),
        ("TempThreshAP1", bitmask(0, 0)),
    ]


class first_errors(error_mask):
    _name = "first_error"
    _file = "first_error"
    _bitmasks = [
        ("TxReqCounterOverflow", bitmask(40, 40)),
        ("TxCh2FifoOverflow", bitmask(33, 33)),
        ("MMIOTimedOut", bitmask(32, 32)),
        ("TxCh1IllegalVCsel", bitmask(25, 25)),
        ("TxCh1NonZeroSOP", bitmask(24, 24)),
        ("TxCh1IncorrectAddr", bitmask(23, 23)),
        ("TxCh1DataPayloadOverrun", bitmask(22, 22)),
        ("TxCh1InsufficientData", bitmask(21, 21)),
        ("TxCh1Len4NotAligned", bitmask(20, 20)),
        ("TxCh1Len2NotAligned", bitmask(19, 19)),
        ("TxCh1Len3NotSupported", bitmask(18, 18)),
        ("TxCh1InvalidReqEncoding", bitmask(17, 17)),
        ("TxCh1Overflow", bitmask(16, 16)),
        ("TxCh0Len4NotAligned", bitmask(4, 4)),
        ("TxCh0Len2NotAligned", bitmask(3, 3)),
        ("TxCh0Len3NotSupported", bitmask(2, 2)),
        ("TxCh0InvalidReqEncoding", bitmask(1, 1)),
        ("TxCh0Overflow", bitmask(0, 0)),
    ]


class nonfatal_errors(error_mask):
    _name = "nonfatal"
    _file = "nonfatal_errors"
    _bitmasks = [
        ("MBPErr", bitmask(12, 12)),
        ("PowerThreshAP2", bitmask(11, 11)),
        ("PowerThreshAP1", bitmask(10, 10)),
        ("TempThreshAP6", bitmask(9, 9)),
        ("InjectedWarningErr", bitmask(6, 6)),
        ("AfuAccessModeErr", bitmask(5, 5)),
        ("ProcHot", bitmask(4, 4)),
        ("PortFatalErr", bitmask(3, 3)),
        ("PcieError", bitmask(2, 2)),
        ("TempThreshAP2", bitmask(1, 1)),
        ("TempThreshAP1", bitmask(0, 0)),
    ]


class fatal_errors(error_mask):
    _name = "catfatal"
    _file = "catfatal_errors"
    _bitmasks = [
        ("InjectedCatastErr", bitmask(11, 11)),
        ("ThermCatastErr", bitmask(10, 10)),
        ("CrcCatastErr", bitmask(9, 9)),
        ("InjectedFatalErr", bitmask(8, 8)),
        ("PciePoisonErr", bitmask(7, 7)),
        ("FabricFatalErr", bitmask(6, 6)),
        ("IommuFatalErr", bitmask(5, 5)),
        ("DramFatalErr", bitmask(4, 4)),
        ("KtiProtoFatalErr", bitmask(3, 3)),
        ("CciFatalErr", bitmask(2, 2)),
        ("TagCchFatalErr", bitmask(1, 1)),
        ("KtiLinkFatalErr", bitmask(0, 0)),
    ]


error_classes = {
    "fme": {"errors": fme_errors,
            "pcie0_errors": pcie0_errors,
            "pcie1_errors": pcie1_errors,
            "gbs_errors": gbs_errors,
            "bbs_errors": bbs_errors,
            "nonfatal_errors": nonfatal_errors,
            "catfatal_errors": fatal_errors,
            "first_error": error_mask,
            "next_error": error_mask},
    "port": {"errors": port_errors,
             "first_error": first_errors}
}


def set_error_parsers(err_feature, err_classes):
    for name, err_class in err_classes.iteritems():
        if hasattr(err_feature, name):
            value = getattr(err_feature, name)
            setattr(err_feature, name, value)


class errors_command(fpga_command):
    name = 'errors'

    def __init__(self, parser):
        super(errors_command, self).__init__(parser)

    def args(self, parser):
        super(errors_command, self).args(parser)
        parser.add_argument('-c', '--clear', action='store_true',
                            default=False,
                            help='specify whether or not'
                            ' to clear error registers')

        parser.add_argument('which',
                            choices=['fme', 'port', 'all'],
                            default='fme',
                            help='specify what kind of errors to operate on')

    def run(self, args):
        info = sysfsinfo()
        resources = []
        # let's monkey patch the error sysfs resource
        # by looking at our dicionaries of error parsers and matching
        # them with the corresponding property and then replacing
        # that property attribute with an instance of the error parsing
        # class with the value of the property
        if args.which == "fme" or args.which == "all":
            for fme in info.fme(**vars(args)):
                set_error_parsers(fme.errors, error_classes["fme"])
                resources.append(fme.errors)

        if args.which == "port" or args.which == "all":
            for port in info.port(**vars(args)):
                set_error_parsers(port.errors, error_classes["port"])
                resources.append(port.errors)

        json_data = []
        for r in resources:
            if args.json:
                json_data.append(r.to_dict(True))
            else:
                r.print_info(
                    "//****** {} ******//".format(r.name()))

        if args.json:
            print(json.dumps(json_data, indent=4, sort_keys=False))

        if args.clear:
            for r in resources:
                if not r.clear():
                    print("ERROR: Could not clear errors for resource {}.\n"
                          "Are you running as root?\n".format(r.sysfs_path))


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    errors_command(parser)
    args = parser.parse_args()
    args.func(args)
