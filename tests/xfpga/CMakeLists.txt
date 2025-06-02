## Copyright(c) 2020-2023, Intel Corporation
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
        ${OPAE_LIB_SOURCE}/plugins/xfpga/bitstream.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/buffer.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/close.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/common.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/enum.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/error.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/event.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/hostif.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/manage.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/mmap.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/mmio.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/open.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/opae_drv.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/properties.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/plugin.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/reconf.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/reset.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/sysfs.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/sysobject.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/umsg.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/userclk.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/version.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/wsid_list.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/usrclk/fpga_user_clk.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/usrclk/fpga_user_clk_type1.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/usrclk/fpga_user_clk_type2.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/metrics_max10.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/metrics_utils.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/afu_metrics.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/metrics.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/vector.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/threshold.c
    LIBS
        ${json-c_LIBRARIES}
        opaeuio
        opae-c
)

opae_test_add_static_lib(TARGET bmc-static
    SOURCE 
        ${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/bmc/bmc.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/bmc/bmc_ioctl.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/bmc/bmcdata.c
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/bmc/bmcinfo.c
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
    LIBS opaeuio
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
        test-fpgad-static
        bmc-static
        ${json-c_LIBRARIES}
        xfpga-static
    TEST_FPGAD
)

target_include_directories(test_xfpga_events_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/libbitstream
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
    SOURCE test_threshold_c.cpp
    LIBS xfpga-static
)

opae_test_add(TARGET test_xfpga_plugin_c
    SOURCE test_plugin_c.cpp
    LIBS xfpga-static
)
