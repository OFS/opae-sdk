#!/usr/bin/env python
# Copyright(c) 2018, Intel Corporation
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
import logging
import time
import uuid
import sys

from opae import properties, token, handle, dma_buffer, OPEN_SHARED, ACCELERATOR


def KB(x): return x*1024


def MB(x): return x*1024*1024


def GB(x): return x*1024*1024*1024


def cacheline_aligned(addr):
    return addr >> 6


NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"
LPBK1_BUFFER_SIZE = MB(1)
LPBK1_BUFFER_ALLOCATION_SIZE = MB(2)
LPBK1_DSM_SIZE = MB(2)
CSR_SRC_ADDR = 0x0120
CSR_DST_ADDR = 0x0128
CSR_CTL = 0x0138
CSR_CFG = 0x0140
CSR_NUM_LINES = 0x0130
DSM_STATUS_TEST_COMPLETE = 0x40
CSR_AFU_DSM_BASEL = 0x0110
CSR_AFU_DSM_BASEH = 0x0114
DSM_TIMEOUT = 1.0


def get_accel(args):
    props = properties()
    props.type = ACCELERATOR
    if args.bus:
        props.bus = args.bus
    if args.guid:
        props.guid = str(uuid.UUID(args.guid))
    tokens = token.enumerate([props])
    if not tokens:
        logging.warning("No resources found using filter")
        sys.exit(-1)
    if len(tokens) > 1:
        logging.warning("Found more than one resourc, using first one")
    mode = 0 if args.exclusive_mode else OPEN_SHARED
    accel = handle.open(tokens[0], mode, 0)
    props = properties.read(tokens[0])
    if not accel:
        logging.error("Could not open accelerator")
        sys.exit(-1)
    return accel, props


def add_arguments(parser):
    parser.add_argument('--begin', type=int, default=1)
    parser.add_argument('--end', type=int, default=1)
    parser.add_argument('--step', type=int, default=1)
    parser.add_argument('--dsm-timeout', type=float, default=1E5)


def run(args, cmdline):
    args.guid = NLB0

    accel, _ = get_accel(args)
    dsm = dma_buffer.allocate(accel, MB(2))
    inp = dma_buffer.allocate(accel, args.end*64)
    out = dma_buffer.allocate(accel, args.end*64)
    logging.debug("Initializing DSM buffer")
    dsm.fill(0)
    logging.debug("Initializing input buffer")
    inp.fill(0xAB)
    logging.debug("Initializing output buffer")
    out.fill(0x1F)
    logging.debug("Resetting accelerator")
    accel.reset()
    accel.write_csr64(CSR_AFU_DSM_BASEL, dsm.iova())
    accel.write_csr32(CSR_CTL, 0)
    accel.write_csr32(CSR_CTL, 1)
    accel.write_csr64(CSR_SRC_ADDR, inp.iova())
    accel.write_csr64(CSR_DST_ADDR, out.iova())
    accel.write_csr32(CSR_CFG, 0x42000)
    errors = 0
    for i in range(args.begin, max(args.begin+args.step, args.end), args.step):
        dsm.fill(0)
        out.fill(0)

        accel.write_csr32(CSR_CTL, 0)
        accel.write_csr32(CSR_CTL, 1)
        logging.debug("CSR_NUM_LINES = %s", i)
        accel.write_csr32(CSR_NUM_LINES, i)
        logging.debug("Starting test")
        accel.write_csr32(CSR_CTL, 3)
        logging.debug("Polling on dsm test complete")
        begin = time.time()
        dsm_timedout = False
        if not dsm.poll(DSM_STATUS_TEST_COMPLETE, 0x1, mask=0x1):
            logging.error(
                "cachelines: %8s : FAIL - Timeout waiting for DSM", i)
            dsm_timedout = True
        accel.write_csr32(CSR_CTL, 7)
        if not dsm_timedout:
            logging.debug(
                "Test complet after: %s seconds",
                time.time()-begin)
            if not all(((inp[j] == out[j]) for j in range(i))):
                logging.error("cachelines: %8s : FAIL - buffers mismatch", i)
                errors += 1
        logging.debug("cachelines: %8s : PASS", i)
