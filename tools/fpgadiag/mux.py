#!/usr/bin/env python
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

import datetime
import itertools
import json
import math
import random
import tempfile
import subprocess
import time

max_threads = 2047;  # 11 bits
max_count = 1048575; # 20 bits
max_stride = 4294967295; # 32 bit

class nlb_options(object):
    all_options = {
            "begin":  range(1, 65535),
            "end":  range(2, 65535),
            "cont" : [True, False],
            "timeout-sec" : range(1,11),
            "mode": ["read", "write"],
            "multi-cl":  [1, 2, 4],
            "cache-policy":  ["wrline-I", "wrline-M", "wrpush-I"],
            "cache-hint": ["rdline-I", "rdline-S"],
            "read-vc":    ["vh0", "vh1"], # vl0
            "write-vc":   ["vh0", "vh1"], # vl0
            "wrfence-vc": ["vh0", "vh1", "none"], # vl0
            "strided-access": range(1,65),
            "threads": range(1,65),
            "count": range(1,100),
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
            opts.update(dict([(k, random.choice(self.all_options[k])) for k in self.keys]))
            opts.update(self.const())
            if self.validate(opts):
                yield opts

    def iter(self):
        values = [self.all_options[k] for k in self.keys]
        for v in itertools.product(*values):
            opts = dict(zip(self.keys, v))
            opts.update(self.const())
            if self.validate(opts):
                yield opts

    def const(self):
        return {}

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
        if int(opts["begin"])%int(opts["multi-cl"]) > 0:
            return False
        return True

class nlb3_options(nlb_options):
    @property
    def keys(self):
        return ("begin",
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
        if int(opts["begin"])%int(opts["multi-cl"]) > 0:
            return False
        if opts.get("end", opts["begin"]) * opts.get("strided-access") > 65535:
            return False
        return True

class mtnlb7_options(nlb_options):
    def const(self):
        return {"mode":"mt7"}

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
        return {"mode":"mt8"}

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

def run_mux(muxfile, iteration=0):
    errcode = 0
    result = "PASS"
    stdout, stderr = None, None
    try:
        dirname = os.path.dirname(os.path.abspath(__file__))
        exename = os.path.join(dirname, 'swmux')
        p = subprocess.Popen('{} -m {}'.format(exename, muxfile), shell=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        timedout = False
        start_time = time.time()
        while p.poll() is None and time.time() - start_time < 30:
            time.sleep(1)
        if p.poll() is None:
            p.terminate()
            result = "TIMEOUT"
            timedout = True
        stderr,stdout = p.communicate()
    except:
        result = "CRASH"
    else:
        if not timedout:
            result = "PASS" if p.returncode == 0 else "FAIL"
    print stdout
    print stderr
    if result == "PASS":
        stdout,stderr = None, None
    result = {"result": result,
              "filename": muxfile,
              "iteration": iteration,
              "stdout" : stdout,
              "stderr" : stderr}

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
            failures +=1
        elif r["result"] == "CRASH":
            crashes += 1
        elif r["result"] == "TIMEOUT":
            timeout +=1
    return {"overview": {"failures":failures,
            "passes": passes,
            "crashes": crashes,
            "timeout" : timeout,
            "total": len(results)},
            "results": results}

def run(generator="iter", **kwargs):
    n0 = nlb0_options()
    n3 = nlb3_options()
    mt7 = mtnlb7_options()
    mt8 = mtnlb8_options()
    engines = { "nlb0" : n0,
                "nlb3" : n3,
                "mtnlb7" : mt7,
                "mtnlb8" : mt8 }
    results = []
    iterators = [getattr(engines[e], generator)() for e in kwargs.get("engines", [])]
    count = 1
    start_time = datetime.datetime.now()
    stop_on_fail = kwargs.get("stop_on_fail", False)
    while True:
        options = [
                { "name" : "nlb0",
                  "app"  : "nlb0",
                  "config" : next(iterators[0]) },
                { "name" : "nlb3",
                  "app"  : "nlb3",
                  "config" : next(iterators[1]) },
                { "name" : "mtnlb7",
                  "app"  : "mtnlb",
                  "config" : next(iterators[2]) },
                { "name" : "mtnlb8",
                  "app"  : "mtnlb",
                  "config" : next(iterators[3]) }
                ]
        for opt in options:
            if opt["name"] in kwargs.get('disable', []):
                opt["disable"] = True

        with tempfile.NamedTemporaryFile("w",
                                         prefix="mux-{}-{}-".format(count, generator),
                                         suffix=".json", delete=False, dir="muxfiles") as fd:
            json.dump(options, fd, indent=4, sort_keys=True)
        print count, "running with: " ,fd.name
        count += 1
        try:
            result = run_mux(fd.name, count)
        except KeyboardInterrupt:
            print "Stopping..."
        else:
            results.append(result)

        if result["result"] != "PASS":
            print "tests failed with file: ", fd.name
            if stop_on_fail:
                break
        if kwargs.get("timeout") is not None:
            if (datetime.datetime.now() - start_time).total_seconds() >  kwargs["timeout"]:
                break
        if kwargs.get("max") is not None:
            if count > kwargs.get("max"):
                break

    with open("mux-results-{}.json".format(datetime.datetime.now().isoformat('_')), "w") as fd_results:
        json.dump(summarize(results), fd_results, indent=4, sort_keys=True)

def retry(results_file, **kwargs):
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
                if config["config"].get("mode") == "mt7" or config["name"] == "nlb3":
                    config["disable"] = False
                else:
                    config["disable"] = True
            rerunfile =  result["filename"].replace("random", "rerun")
            print rerunfile
            with open(rerunfile, "w") as fd:
                json.dump(configs, fd, indent=4, sort_keys=True)
            retries.append(run_mux(rerunfile, result["iteration"]))
    with open(results_file.replace("results", "retries"), "w") as fd:
        json.dump(summarize(retries), fd, indent=4, sort_keys=True)





if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-g", "--generator", default="iter", choices=["iter", "random"],
                        help="generator type for generating options")
    parser.add_argument("-m", "--max-iterations", type=int,
                        help="maximum number of iterations to execute")
    parser.add_argument("-t", "--timeout", type=float,
                        help="stop running after timeout period has elapsed")
    parser.add_argument("--stop-on-fail", action="store_true", default=False,
                        help="stop running after first failure")
    parser.add_argument("--retry",
                        help="run failed cases from a results file")
    parser.add_argument("--engines", nargs="+", default=["nlb0", "nlb3", "mtnlb7", "mtnlb8"])
    parser.add_argument("--disable", nargs="*", default=[])
    args = parser.parse_args()
    if not os.path.exists("muxfiles"):
        os.mkdir("muxfiles")

    if args.retry:
        retry(args.retry)
    else:
        run(args.generator, max=args.max_iterations, timeout=args.timeout, stop_on_fail=args.stop_on_fail, engines=args.engines, disable=args.disable)
