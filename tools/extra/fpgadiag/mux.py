#!/usr/bin/env python3
# Copyright(c) 2017, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#  this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
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
from __future__ import absolute_import
from __future__ import print_function
import datetime
import itertools
import json
import math
import random
import tempfile
import subprocess
import os

max_threads = 2047       # 11 bits
max_count = 1048575      # 20 bits
max_stride = 4294967295  # 32 bit


class nlb_options(object):
    all_options = {
        "begin": list(range(1, 65535)),
        "end": list(range(2, 65535)),
        "cont": [True, False],
        "timeout-sec": list(range(1, 11)),
        "mode": ["read", "write"],
        "multi-cl": [1, 2, 4],
        "cache-policy": ["wrline-I", "wrline-M", "wrpush-I"],
        "cache-hint": ["rdline-I", "rdline-S"],
        "read-vc": ["vh0", "vh1", "vl0"],
        "write-vc": ["vh0", "vh1"],
        "wrfence-vc": ["vh0", "vh1"],
        "strided-access": list(range(1, 65)),
        "threads": list(range(1, 65)),
        "count": list(range(1, 100)),
        "stride": [int(math.pow(2, n)) for n in range(9)],
        "warm-fpga-cache": [True, False],
        "cool-fpga-cache": [True, False],
        "cool-cpu-cache": [True, False],
        "alt-wr-pattern": [True, False]
    }

    @property
    def keys(self):
        return ()

    def empty(self):
        while True:
            yield {}

    def random(self):
        while True:
            opts = {}
            opts.update(
                dict([(k, random.choice(self.all_options[k]))
                      for k in self.keys]))
            opts.update(self.const())
            if self.validate(opts):
                yield opts

    def iter(self):
        values = [self.all_options[k] for k in self.keys]
        for v in itertools.product(*values):
            opts = dict(list(zip(self.keys, v)))
            opts.update(self.const())
            if self.validate(opts):
                yield opts

    def const(self):
        return {'suppress-stats': True}

    def validate(self, opts):
        return True


class nlb0_options(nlb_options):

    @property
    def keys(self):
        return ("begin",
                "cont",
                "timeout-sec",
                "multi-cl",
                "cache-policy",
                "cache-hint",
                "read-vc",
                "write-vc",
                "wrfence-vc")

    def validate(self, opts):
        if int(opts["begin"]) % int(opts["multi-cl"]) > 0:
            return False
        return True


class nlb3_options(nlb_options):

    @property
    def keys(self):
        return ("begin",
                "end",
                "mode",
                "cont",
                "timeout-sec",
                "multi-cl",
                "strided-access",
                "cache-policy",
                "cache-hint",
                "read-vc",
                "write-vc",
                "wrfence-vc",
                "alt-wr-pattern",
                "warm-fpga-cache",
                "cool-fpga-cache",
                "cool-cpu-cache")

    def validate(self, opts):
        if opts["warm-fpga-cache"] and opts["cool-fpga-cache"]:
            return False
        if int(opts["begin"]) % int(opts["multi-cl"]) > 0:
            return False
        if int(opts["end"]) % int(opts["multi-cl"]) > 0:
            return False
        if opts.get("end") < opts.get("begin"):
            return False
        if opts.get("end", opts["begin"]) * opts.get("strided-access") > 65535:
            return False
        return True

    def const(self):
        return {'cont': False}


class mtnlb7_options(nlb_options):

    def const(self):
        return {"mode": "mt7", "suppress-stats": True}

    @property
    def keys(self):
        return ("threads",
                "count",
                "stride",
                "cache-policy",
                "cache-hint",
                "read-vc",
                "write-vc",
                )


class mtnlb8_options(nlb_options):

    def const(self):
        return {"mode": "mt8", "suppress-stats": True}

    @property
    def keys(self):
        return ("threads",
                "count",
                "stride",
                "cache-policy",
                "cache-hint",
                "read-vc",
                "write-vc",
                )

    def validate(self, options):
        if options["threads"] > max_threads:
            return False
        if options["count"] > max_count:
            return False
        if options["stride"] > max_stride:
            return False
        return True


def run_mux(muxfile, args, iteration=0):
    result = "PASS"
    stdout, stderr = None, None
    try:
        p = subprocess.Popen(
            '{} -c {} -s {}'.format(args.cmd, muxfile, args.socket_id),
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        timedout = False
        stdout, stderr = p.communicate()
    except subprocess.CalledProcessError:
        result = "CRASH"
    else:
        if not timedout:
            result = "PASS" if p.returncode == 0 else "FAIL"
    if result == "PASS":
        stdout, stderr = None, None
    result = {"result": result,
              "filename": muxfile,
              "iteration": iteration,
              "stdout": stdout,
              "stderr": stderr}

    return result


def summarize(results):
    failures = 0
    passes = 0
    crashes = 0
    timeout = 0
    for r in results:
        if r["result"] == "PASS":
            passes += 1
        elif r["result"] == "FAIL":
            failures += 1
        elif r["result"] == "CRASH":
            crashes += 1
        elif r["result"] == "TIMEOUT":
            timeout += 1
    return {"overview": {"failures": failures,
                         "passes": passes,
                         "crashes": crashes,
                         "timeout": timeout,
                         "total": len(results)},
            "results": results}


def run(args):
    generator = args.generator
    n0 = nlb0_options()
    n3 = nlb3_options()
    mt7 = mtnlb7_options()
    mt8 = mtnlb8_options()
    engines = {"nlb0": n0,
               "nlb3": n3,
               "mtnlb7": mt7,
               "mtnlb8": mt8}
    results = []
    count = 1
    start_time = datetime.datetime.now()
    stop_on_fail = args.stop_on_fail
    while True:
        options = [{"name": n,
                    "app": n,
                    "config": next(getattr(engines[n], generator)())}
                   for n in args.engines]
        for opt in options:
            if opt["name"] in args.disable:
                opt["disabled"] = True
        if len(options) == 1:
            file_prefix = options[0]["name"]
            options = options[0]["config"]
        with tempfile.NamedTemporaryFile("w",
                                         prefix=file_prefix,
                                         suffix=".json",
                                         delete=False,
                                         dir="muxfiles") as fd:
            json.dump(options, fd, indent=4, sort_keys=True)
        print(count, "running with: ", fd.name)
        count += 1
        try:
            result = run_mux(fd.name, args, count)
        except KeyboardInterrupt:
            print("Stopping...")
            return results
            break
        else:
            results.append(result)

        if result["result"] != "PASS":
            print("tests failed with file: ", fd.name)
            if stop_on_fail:
                break
        if args.timeout is not None:
            total_sec = (datetime.datetime.now() - start_time).total_seconds()
            if total_sec > args.timeout:
                break
        if args.max_iterations is not None:
            if count >= args.max_iterations:
                break
    return results


def retry(args):
    results_file = args.retry
    results = []
    retries = []
    with open(results_file, "r") as fd:
        results = json.load(fd)
    for result in results["results"]:
        if result["result"] != "PASS":
            with open(result["filename"], "r") as fd:
                configs = json.load(fd)
            for config in configs:
                config["app"] = config["name"]
                if config["config"].get("mode") == "mt7":
                    config["name"] = "mtnlb7"
                elif config["config"].get("mode") == "mt8":
                    config["name"] = "mtnlb8"
                config["disable"] = True
            rerunfile = result["filename"].replace("random", "rerun")
            print(rerunfile)
            with open(rerunfile, "w") as fd:
                json.dump(configs, fd, indent=4, sort_keys=True)
            retries.append(run_mux(rerunfile, args, result["iteration"]))
    return retries


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("cmd")
    parser.add_argument(
        '-s', '--socket-id', default=0, choices=[0, 1], type=int)
    parser.add_argument(
        "-g", "--generator", default="iter", choices=["iter", "random"],
        help="generator type for generating options")
    parser.add_argument("-m", "--max-iterations", type=int,
                        help="maximum number of iterations to execute")
    parser.add_argument("-t", "--timeout", type=float,
                        help="stop running after timeout period has elapsed")
    parser.add_argument("--stop-on-fail", action="store_true", default=False,
                        help="stop running after first failure")
    parser.add_argument("--retry",
                        help="run failed cases from a results file")
    parser.add_argument("--engines", nargs="+",
                        default=["nlb0", "nlb3", "mtnlb7", "mtnlb8"])
    parser.add_argument("--disable", nargs="*", default=[])
    args = parser.parse_args()
    if not os.path.exists("muxfiles"):
        os.mkdir("muxfiles")

    if args.retry:
        retries = retry(args)
        with open(args.retry.replace("results", "retries"), "w") as fd:
            json.dump(summarize(retries), fd, indent=4, sort_keys=True)
    else:
        results = run(args)
        datestr = datetime.datetime.now().isoformat('_')
        fname = "mux-results-{}.json".format(datestr)
        with open(fname, "w") as fd_results:
            json.dump(summarize(results), fd_results, indent=4, sort_keys=True)
