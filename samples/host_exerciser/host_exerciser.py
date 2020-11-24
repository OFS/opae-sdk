#!/usr/bin/env python3
# Copyright(c) 2020, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
#
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

import argparse
import grp
import os
import pwd
import subprocess
import sys
import time

def parse_args():
    """Parse script arguments."""

    parser = argparse.ArgumentParser()

    parser.add_argument('-i', '--device-id',
                        type=str,
                        help='Device Id for Intel FPGA')

    parser.add_argument('-B', '--bus', type=str,
                        help='Bus number for specific FPGA')

    parser.add_argument('-D', '--device', type=str,
                        help='Device number for specific FPGA')

    parser.add_argument('-F', '--function', type=str,
                        help='Function number for specific FPGA')

    parser.add_argument('gbs_paths', nargs='*', type=str,
                        help='Paths for the gbs files for BIST')

    args = parser.parse_args()

    # Bist defaulted to hex values for BDF. Interpret the strings and
    # replace them with integers.
    if args.device_id:
        args.device_id = int(args.device_id, 16)
    if args.bus:
        args.bus = int(args.bus, 16)
    if args.device:
        args.device = int(args.device, 16)
    if args.function:
        args.function = int(args.function, 16)

    return args

def load_gbs(cmd_input,args):
    """Partial Reconfiguration bitstream."""

    print("Attempting Partial Reconfiguration:")
    print("cmd_input:",cmd_input)
    cmd =  cmd_input.format(hex(args.bus),0,0,args.gbs_paths)
    print("cmd:",cmd)
    try:
        subprocess.check_call(cmd.split(' '))
    except subprocess.CalledProcessError as e:
        print("Failed to load gbs file: {}".format(args.gbs_paths))
        print("Please try a different gbs")
        print(e)


class HostExerciser(object):

    def __init__(self, args):
         self.args =args

    host_exerciser_list = \
                {'lpbk':     'host_exerciser -p {:02x}:{:02d}.{:01d} lpbk',
                'mem':       'host_exerciser -p {:02x}:{:02d}.{:01d} mem',
                'reconf':    'fpgaconf -B {} -D {:02d} -F {:01d} -V {}',
                'lpbk mem':  'host_exerciser -p {:02x}:{:02d}.{:01d} mem',
                'hssi':      'hssi -p {:02x}:{:02d}.{:01d} hssi_100g'
                }

    def run(self):
        """runs host exerciser list commands"""
        print("----RUN----")
        print(self.host_exerciser_list)
        print(self.args)
        print(args.bus)
        print(args.device)
        print(args.function)
        for key, value in self.host_exerciser_list.items():
            print(key, ' : ', value)
            print("\n \n CMD:",value)
            if value.find('fpgaconf') != -1:
               print("Found fpgaconf")
               load_gbs(value,args)
               continue 
            else :
                cmd =  value.format(args.bus,0,0)
                print(cmd)
                try:
                    subprocess.check_call(cmd.split(' '))
                except subprocess.CalledProcessError as e:
                    print("Failed Test: {}")
                    print(e)
        return True


def main(args):
    """ Main Host script function."""

    print("HOST Execiser PF Tester")
    he = HostExerciser(args)
    he.run()


if __name__ == "__main__":
    args = parse_args()
    print(args)
    main(args)
