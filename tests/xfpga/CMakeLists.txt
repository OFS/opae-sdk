## Copyright(c) 2020, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

opae_test_add_static_lib(TARGET xfpga-static
    SOURCE
        ${OPAE_LIBS_ROOT}/plugins/xfpga/bitstream.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/buffer.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/close.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/common.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/enum.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/error.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/event.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/hostif.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/init.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/manage.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/mmap.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/mmio.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/open.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/opae_drv.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/properties.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/plugin.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/reconf.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/reset.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/sysfs.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/sysobject.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/token_list.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/umsg.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/userclk.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/version.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/wsid_list.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/usrclk/user_clk_pgm_uclock.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/metrics_max10.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/metrics_utils.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/afu_metrics.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/metrics.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/vector.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/threshold.c
    LIBS
        ${libjson-c_LIBRARIES}
        opae-c
)

opae_test_add_static_lib(TARGET bmc-static
    SOURCE 
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/bmc/bmc.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/bmc/bmc_ioctl.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/bmc/bmcdata.c
        ${OPAE_LIBS_ROOT}/plugins/xfpga/metrics/bmc/bmcinfo.c
)

opae_test_add(TARGET test_xfpga_token_list_c
    SOURCE test_token_list_c.cpp
    LIBS
        xfpga-static
)

opae_test_add(TARGET test_xfpga_error_c
    SOURCE test_error_c.cpp
    LIBS xfpga-static
)

set_tests_properties(test_xfpga_error_c
    PROPERTIES ENVIRONMENT "LIBOPAE_LOG=1")
                     
opae_test_add(TARGET test_xfpga_mock_errinj_c
    SOURCE test_mock_errinj_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_common_c
    SOURCE test_common_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_properties_c
    SOURCE test_properties_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_object_c
    SOURCE test_object_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_enum_c
    SOURCE test_enum_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_buffer_c
    SOURCE test_buffer_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_wsid_list_c
    SOURCE test_wsid_list_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_sysfs_c
    SOURCE test_sysfs_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_mmio_c
    SOURCE test_mmio_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_metadata_c
    SOURCE test_metadata_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_reconf_c
    SOURCE test_reconf_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_version_c
    SOURCE test_version_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_open_close_c
    SOURCE test_open_close_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_usrclk_c
    SOURCE test_usrclk_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_reset_c
    SOURCE test_reset_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_umsg_c
    SOURCE test_umsg_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_manage_c
    SOURCE test_manage_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_events_c
    SOURCE test_events_c.cpp
    LIBS
        safestr
        test-fpgad-static
        bmc-static
        ${libjson-c_LIBRARIES}
        xfpga-static
    TEST_FPGAD
)

target_include_directories(test_xfpga_events_c
    PRIVATE
        ${OPAE_LIBS_ROOT}/libbitstream
)

opae_test_add(TARGET test_xfpga_metrics_vector_c
    SOURCE test_metrics_vector_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_metrics_utils_c
    SOURCE test_metrics_utils_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_afu_metrics_c
    SOURCE test_afu_metrics_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_metrics_c
    SOURCE test_metrics_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_bmc_c
    SOURCE test_bmc_c.cpp
    LIBS
        bmc-static
        xfpga-static
)

opae_test_add(TARGET test_xfpga_max10_c
    SOURCE test_metrics_max10_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_threshold_c
    SOURCE test_thresold_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_plugin_c
    SOURCE test_plugin_c.cpp
    LIBS xfpga-static
)
