#
# Copyright (c) 2018, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of the Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# Utilities for emitting configuration, invoked by afu_platform_config.
#

from __future__ import absolute_import
from __future__ import print_function
import os
import sys
import re


def errorExit(msg):
    sys.stderr.write("\nError: " + msg + "\n")
    sys.exit(1)


def emitHeader(f, afu_ifc_db, platform_db, comment="//"):
    f.write(comment + "\n" +
            comment + " This file has been generated automatically by " +
            "afu_platform_config.\n" +
            comment + " Do not edit it by hand.\n" +
            comment + "\n")
    f.write(comment + " Platform: {0}\n".format(platform_db['file_name']))
    f.write(comment + " AFU top-level interface: {0}\n{1}\n\n".format(
        afu_ifc_db['file_name'], comment))


def getIntendedDevFamily(platform_db):
    try:
        f = platform_db['fpga-family']
    except KeyError:
        # Older platform databases didn't include fpga-family
        f = 'A10'

    # Map OPAE platform_db families to megafunction names.
    families = dict()
    families['A'] = 'Arria'
    families['S'] = 'Stratix'

    # Is the encoded fpga-family a single letter followed by a number?
    if ((f[0] in families) and f[1].isdigit()):
        # Yes -- return the expanded family and the version number
        return families[f[0]] + f[1:]

    return "Unknown"


#
# Compute derived parameter values based on the database.  These
# are typically computations more easily done here in Python than
# at synthesis time in Verilog.
#
def computedParams(afu_port, params):
    # The script used to accept "auto" for add-timing-reg-stages.
    # This is no longer necessary, since the number of stages expected
    # for a given interface class will be a fixed, platform-independent
    # value.
    try:
        n_stages = params['add-timing-reg-stages']
        if (n_stages == 'auto'):
            n_stages = 0
            params['add-timing-reg-stages'] = 0

        # Raise ValueError if the parameter isn't a positive integer
        if (int(n_stages) < 0):
            raise ValueError
    except KeyError:
        None
    except ValueError:
        errorExit(afu_port['class'].upper() +
                  ' parameter add-timing-reg-stages must be ' +
                  'either an unsigned integer or "auto"')

    if (afu_port['class'].upper() == 'CCI-P'):
        # Convert the 'vc-supported' vector into a count of channels.
        try:
            vc = params['vc-supported']
            # It starts as a string.
            vc = re.sub('[{} ]', '', vc)
            # Count of boolean (0 or 1) is channel supported entries
            vc = [int(x) for x in vc.split(',')]
            # Ignore VA
            vc[0] = 0
            cnt = sum(vc)
        except KeyError:
            cnt = 0
        # Add the count as a new parameter
        params['num-phys-channels'] = cnt

        # Default channel is VA unless there is only one channel active.
        # With one channel active, this Python will indicate which one
        # to use.
        if (cnt != 1):
            chan = 0
        else:
            chan = vc.index(1)
        params['vc-default'] = chan


#
# Verilog's preprocessor is unimpressive.  Among its failings is poor handling
# of strings.  Some of the definitions written by emitConfig are easier to
# handle as tags.
#
is_params = set()
is_params.add('clock')


#
# Platform shims transform the top-level interface before signals reach
# an AFU top-level interface.  These are the parameters that control the
# transformation.  They are the only parameters that may be overridden
# in an AFU JSON module-ports section.
#
platform_shim_params = set()
platform_shim_params.add('clock')
platform_shim_params.add('addr-width')
platform_shim_params.add('data-width')
platform_shim_params.add('add-timing-reg-stages')


#
# Some platform_shim_params can be validated here.  These parameters must
# be less than or equal to the value from the platform.
#
platform_shim_le_params = set()
platform_shim_le_params.add('addr-width')
platform_shim_le_params.add('data-width')


#
# Emit the Verilog header file with the AFU interface and
# platform capabilities.
#
def emitConfig(args, afu_ifc_db, platform_db, platform_defaults_db,
               afu_port_list):
    # Path prefix for emitting configuration files
    f_prefix = ""
    if (args.tgt):
        f_prefix = args.tgt

    #
    # platform_afu_top_config.vh describes the required module ports using
    # Verilog preprocessor variables.  A top-level module provided with
    # each platform must honor this configuration.  The platform JSON
    # port options must match the platform's top-level module.
    #
    fn = os.path.join(f_prefix, "platform_afu_top_config.vh")
    if (not args.quiet):
        print("Writing {0}".format(fn))

    # Regular expression for characters we might encounter that can't be
    # in preprocessor variables.  They will all be replaced with underscores.
    illegal_chars = re.compile('[\\.\\[\\]-]')

    try:
        f = open(fn, "w")
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn))

    emitHeader(f, afu_ifc_db, platform_db)

    f.write("`ifndef __PLATFORM_AFU_TOP_CONFIG_VH__\n" +
            "`define __PLATFORM_AFU_TOP_CONFIG_VH__\n\n")

    f.write("`define PLATFORM_CLASS_NAME \"" +
            platform_db['platform-name'].upper() + "\"\n")
    f.write("`define PLATFORM_CLASS_NAME_IS_" +
            platform_db['platform-name'].upper() + " 1\n\n")

    f.write("// This may be passed as the \"intended_device_family\"\n" +
            "// parameter to simulated megafunctions.\n")
    f.write("`define PLATFORM_INTENDED_DEVICE_FAMILY \"" +
            getIntendedDevFamily(platform_db) + "\"\n\n")

    f.write("`define AFU_TOP_MODULE_NAME " +
            afu_ifc_db['module-name'] + "\n")
    f.write("`define PLATFORM_SHIM_MODULE_NAME " +
            afu_ifc_db['platform-shim-module-name'] + "\n\n")

    if (args.sim):
        f.write("`define PLATFORM_SIMULATED 1\n\n")

    # Does either the AFU or platform request some preprocessor
    # definitions?
    for d in (platform_db['define'] + afu_ifc_db['define']):
        f.write("`define {0} 1\n".format(d))

    f.write("\n\n// These top-level port classes are provided\n")
    for port in afu_port_list:
        afu_port = port['afu']
        name = "PLATFORM_PROVIDES_" + afu_port['class'].upper()
        name = illegal_chars.sub('_', name)
        f.write("`define " + name + " 1\n")

    f.write("\n\n//\n// These top-level ports are passed from the " +
            "platform to the AFU\n//\n\n")
    for port in afu_port_list:
        afu_port = port['afu']
        plat_port = port['plat']

        f.write("// {0}\n".format(afu_port['class']))

        name = afu_port['class'].upper() + "_" + \
            afu_port['interface'].upper()
        name = illegal_chars.sub('_', name)

        f.write("`define AFU_TOP_REQUIRES_" + name + " ")
        if ('num-entries' not in port):
            f.write("1\n")
        else:
            f.write("{0}\n".format(port['num-entries']))

        if (afu_port['vector']):
            f.write("`define PLATFORM_PARAM_" + name + "_IS_VECTOR 1\n")

        # Does either the AFU or platform request some preprocessor
        # definitions?
        for d in (afu_port['define'] + plat_port['define']):
            f.write("`define {0} 1\n".format(d))

        # Gather the parameters required by this class.  Start with
        # the defaults and then merge in any specified by the platform.
        key = afu_port['class']
        params = dict()
        if (key in platform_defaults_db['module-port-params']):
            # Default values
            params = platform_defaults_db['module-port-params'][key]

        # Platform-specific values in 'params' key within a class
        if ('params' in plat_port):
            for k in plat_port['params'].keys():
                params[k] = plat_port['params'][k]

        # Update parameters overridden by the AFU
        if ('params' in afu_port):
            for k in afu_port['params'].keys():
                if (k not in params):
                    # AFU can't define a new parameter.  It may only update
                    # existing ones.
                    errorExit(
                        ('AFU attempting to set non-existent "{0}" ' +
                         'parameter "{1}"').format(key, k))
                # Only a handful of parameters may be updated by the AFU
                # JSON since most are raw properties of the platform.
                # These special parameters are managed by a shim.
                if (k not in platform_shim_params):
                    errorExit(
                        ('Illegal AFU attempt to override "{0}" ' +
                         'parameter "{1}"').format(key, k))

                # The parameter may be overridden. Is the value legal?
                if ((k in platform_shim_le_params) and
                        (afu_port['params'][k] > params[k])):
                    errorExit(
                        ('Illegal AFU parameter override value "{0}" ' +
                         'parameter "{1}" ({2} > {3})').format(
                             key, k, afu_port['params'][k], params[k]))
                if ((k in platform_shim_le_params) and
                        (afu_port['params'][k] <= 0)):
                    errorExit(
                        ('Illegal AFU parameter override value "{0}" ' +
                         'parameter "{1}" ({2} <= 0)').format(
                             key, k, afu_port['params'][k]))

                params[k] = afu_port['params'][k]

        # Add device-specific computed parameters
        computedParams(afu_port, params)

        # Now we have the parameters.  Emit them.
        for k in sorted(params.keys()):
            name = "PLATFORM_PARAM_" + \
                afu_port['class'].upper() + "_" + k.upper()
            name = illegal_chars.sub('_', name)
            # Skip comments and parameters with no value
            if ((k != 'comment') and (params[k] is not None)):
                f.write('`define {0} {1}\n'.format(name, params[k]))
                if (k in is_params):
                    p = illegal_chars.sub('_', params[k].upper())
                    f.write('`define {0}_IS_{1} 1\n'.format(name, p))

        f.write("\n")

    f.write("\n`endif // __PLATFORM_AFU_TOP_CONFIG_VH__\n")
    f.close()


#
# Emit simulator Makefile/CMake configuration.
#
def emitSimMakeConfig(args, afu_ifc_db, platform_db):
    # Path prefix for emitting configuration files
    f_prefix = ""
    if (args.tgt):
        f_prefix = args.tgt

    #
    # ase_platform_config.mk and ase_platform_config.cmake hold make and
    # cmake preprocessor variables describing the configuration.
    #
    fn_mk = os.path.join(f_prefix, "ase_platform_config.mk")
    fn_cm = os.path.join(f_prefix, "ase_platform_config.cmake")
    if (not args.quiet):
        print("Writing {0} and {1}".format(fn_mk, fn_cm))

    try:
        f_mk = open(fn_mk, "w")
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn_mk))

    try:
        f_cm = open(fn_cm, "w")
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn_cm))

    f_mk.write("# This file has been generated automatically by " +
               "afu_platform_config.\n\n")
    f_cm.write("# This file has been generated automatically by " +
               "afu_platform_config.\n\n")

    # Does either the AFU or platform request some preprocessor
    # definitions?
    for d in (platform_db['define'] + afu_ifc_db['define']):
        f_mk.write("{0} = 1\n".format(d))
        f_cm.write("set({0} 1)\n".format(d))

    f_mk.close()
    f_cm.close()


#
# Emit the QSF script to load the platform interface.
#
def emitQsfConfig(args, afu_ifc_db, platform_db, platform_defaults_db,
                  afu_port_list):
    # Path prefix for emitting configuration files
    f_prefix = ""
    if (args.tgt):
        f_prefix = args.tgt

    #
    # platform_if_addenda.txt imports the platform configuration into
    # the simulator.
    #
    fn = os.path.join(f_prefix, "platform_if_addenda.qsf")
    if (not args.quiet):
        print("Writing {0}".format(fn))

    # Regular expression for characters we might encounter that can't be
    # in preprocessor variables.  They will all be replaced with underscores.
    illegal_chars = re.compile('[\\.\\[\\]-]')

    try:
        f = open(fn, "w")
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn))

    emitHeader(f, afu_ifc_db, platform_db, comment="##")

    f.write('namespace eval platform_cfg {\n')
    f.write("    variable PLATFORM_CLASS_NAME \"" +
            platform_db['platform-name'] + "\"\n")

    f.write("\n    # These top-level port classes are provided\n")
    for port in afu_port_list:
        afu_port = port['afu']
        name = "PLATFORM_PROVIDES_" + afu_port['class'].upper()
        name = illegal_chars.sub('_', name)
        f.write("    variable " + name + " 1\n")

    f.write("\n    # These top-level ports are passed from the " +
            "platform to the AFU\n")
    for port in afu_port_list:
        afu_port = port['afu']
        plat_port = port['plat']

        name = "AFU_TOP_REQUIRES_" + \
            afu_port['class'].upper() + "_" + afu_port['interface'].upper()
        name = illegal_chars.sub('_', name)

        f.write("    variable " + name + " ")
        if ('num-entries' not in port):
            f.write("1\n")
        else:
            f.write("{0}\n".format(port['num-entries']))
    f.write('}\n\n')

    f.write(
        "set_global_assignment -name SOURCE_TCL_SCRIPT_FILE " +
        "{0}/par/platform_if_addenda.qsf\n".format(args.platform_if))
    f.close()


#
# Emit the RTL simulator file to include platform interfaces.
#
def emitSimConfig(args, afu_ifc_db, platform_db, platform_defaults_db,
                  afu_port_list):
    # Path prefix for emitting configuration files
    f_prefix = ""
    if (args.tgt):
        f_prefix = args.tgt

    #
    # platform_if_addenda.txt imports the platform configuration into
    # the simulator.
    #
    fn = os.path.join(f_prefix, "platform_if_addenda.txt")
    if (not args.quiet):
        print("Writing {0}".format(fn))

    try:
        f = open(fn, "w")
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn))

    emitHeader(f, afu_ifc_db, platform_db)

    f.write("-F {0}/sim/platform_if_addenda.txt\n".format(args.platform_if))
    f.close()

    #
    # platform_if_includes.txt just sets up include file paths for
    # the simulator.
    #
    fn = os.path.join(f_prefix, "platform_if_includes.txt")
    if (not args.quiet):
        print("Writing {0}".format(fn))

    try:
        f = open(fn, "w")
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn))

    emitHeader(f, afu_ifc_db, platform_db)

    f.write("-F {0}/sim/platform_if_includes.txt\n".format(args.platform_if))

    # Legacy AFUs may need INCLUDE_DDR4 defined without having to include
    # platform_if.vh.  If INCLUDE_DDR4 is defined, then force it here.
    for port in afu_port_list:
        afu_port = port['afu']
        plat_port = port['plat']
        if ('INCLUDE_DDR4' in afu_port['define'] or
                'INCLUDE_DDR4' in plat_port['define']):
            f.write('+define+INCLUDE_DDR4\n')

    f.close()

    #
    # ase_platform_name.txt holds the name of the corresponding ASE platform.
    #
    fn = os.path.join(f_prefix, "ase_platform_name.txt")
    if (not args.quiet):
        print("Writing {0}".format(fn))

    try:
        f = open(fn, "w")
        f.write(platform_db['ase-platform'] + '\n')
    except KeyError:
        errorExit("Platform {0} does not define ase-platform".format(
            platform_db['file_path']))
    except Exception:
        errorExit("failed to open {0} for writing.".format(fn))

    f.close()

    #
    # Emit some preprocessor configuration for the simulator environment.
    #
    emitSimMakeConfig(args, afu_ifc_db, platform_db)
