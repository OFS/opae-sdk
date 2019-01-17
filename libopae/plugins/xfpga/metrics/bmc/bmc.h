// Copyright(c) 2018, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/*
 * @file fmeinfo.h
 *
 * @brief
 */
#ifndef BMC_H
#define BMC_H

#include <opae/fpga.h>
#include "bmc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load Sensor Data Records from the BMC on the FPGA.
 *
 * Query the BMC for descriptions of all sensors it supports.
 *
 * @note This call creates a new SDR object and allocates memory for it. It
 * is the responsibility of the using application to free this memory after use
 * by calling bmcDestroySDRs() for the SDR object.
 *
 * @note Calling this function with a NULL 'records' parameter will return
 * the total number of sensors available in 'num_sensors'.
 *
 * @param[in] token           fpga_token object for device (FPGA_DEVICE type)
 * @param[out] records        bmc_sdr_handle pointer.  Can be NULL
 * @param[inout] num_sensors  The number of sensors available from the BMC
 * @returns                   FPGA_OK on success
 */
fpga_result bmcLoadSDRs(fpga_token token, bmc_sdr_handle *records,
			uint32_t *num_sensors);

/**
 * Free Sensor Data Records memory allocated by bmcLoadSDRs.
 *
 * @note The 'records' value will be set to NULL on success.
 *
 * @param[in] records        bmc_sdr_handle pointer
 * @returns                  FPGA_OK on success
 */
fpga_result bmcDestroySDRs(bmc_sdr_handle *records);

/**
 * Load raw sensor readings from the BMC on the FPGA.
 *
 * Read all the raw sensor values from the BMC.
 *
 * @note This call creates a new Values object and allocates memory for it. It
 * is the responsibility of the using application to free this memory after use
 * by calling bmcDestroySensorValues() for the Values object.
 *
 * @note This call obtains a snapshot of all the sensor readings, providing
 * readings consistent with the time the function was called.
 *
 * @note Calling this function with a NULL 'values' parameter will return
 * the total number of sensor readings available in 'num_values'.
 *
 * @param[in] records         bmc_sdr_handle pointer
 * @param[in] values          bmc_values_handle pointer (can be NULL)
 * @param[inout] num_values   The number of sensor readings available from the
 * BMC
 * @returns                   FPGA_OK on success
 */
fpga_result bmcReadSensorValues(bmc_sdr_handle records,
				bmc_values_handle *values,
				uint32_t *num_values);

/**
 * Free Sensor Values memory allocated by bmcReadSensorValues.
 *
 * @note The 'values' value will be set to NULL on success.
 *
 * @param[in] values         bmc_values_handle pointer
 * @returns                  FPGA_OK on success
 */
fpga_result bmcDestroySensorValues(bmc_values_handle *values);

/**
 * Return a properly scaled value for a sensor.  The value will be appropriate
 * for comparison or printing in the units specified for the sensor.
 *
 * @note The raw value of the sensor (for linear sensors) is calculated using
 * the "Mx + B * 10^exp" formula and is scaled by the specified result exponent.
 *
 * @note The accuracy and tolerance values are sensor-dependent and may not
 * be supplied.  If they are not supplied, the default value is zero for both.
 * Tolerance is in number of 1/2 raw sensor reading increments.  The accuracy
 * is given in units of 1/100%, scaled up by the accuracy exponent (i.e.,
 * *10^exp).
 *
 * @param[in] values          bmc_values_handle
 * @param[in] sensor_number   The sensor of interest
 * @param[out] is_valid       This sensor's reading is unavailable if 0
 * @param[out] value          The scaled value of this sensor's reading
 * @returns                   FPGA_OK on success
 */
fpga_result bmcGetSensorReading(bmc_values_handle values,
				uint32_t sensor_number, uint32_t *is_valid,
				double *value);

/**
 * Return a list of sensors whose thresholds are reported as exceeded by the
 * BMC.
 *
 * @note This call allocates an array of tripped_thresholds structures of
 * .size 'num_tripped'. It is the responsibility of the using application to
 * free this memory after use by calling bmcDestroyTripped() for the 'tripped'
 * object.
 *
 * @param[in] values          bmc_values_handle
 * @param[in] tripped         Address where a pointer to an array of
 *                            tripped_thresholds structs is stored
 *                            (may be NULL)
 * @param[out] num_tripped    The number of sensors tripped
 * @returns                   FPGA_OK on success
 */
fpga_result bmcThresholdsTripped(bmc_values_handle values,
				 tripped_thresholds **tripped,
				 uint32_t *num_tripped);

/**
 * Free Sensor Values memory allocated by bmcThresholdsTripped.
 *
 * @note The 'tripped' value will be set to NULL on success.
 *
 * @param[in] tripped        bmc_values_handle pointer
 * @returns                  FPGA_OK on success
 */
fpga_result bmcDestroyTripped(tripped_thresholds *tripped);

/**
 * Return detailed information regarding a sensor.
 *
 * @param[in] values          bmc_values_handle
 * @param[in] sensor_number   The sensor of interest
 * @param[out] details        A pointer to a sdr_details structure
 * @returns                   FPGA_OK on success
 */
fpga_result bmcGetSDRDetails(bmc_values_handle values, uint32_t sensor_number,
			     sdr_details *details);

/**
 * Return BMC reported microcontroller version.
 *
 * @param[in] token           fpga_token object for device (FPGA_DEVICE type)
 * @param[out] version        BMC microcontroller version
 * @returns                   FPGA_OK on success
 */
fpga_result bmcGetFirmwareVersion(fpga_token token, uint32_t *version);

/**
 * Return BMC reported last powerdown cause.
 *
 * @param[in] token           fpga_token object for device (FPGA_DEVICE type)
 * @param[out] cause          String describing last powerdown cause
 * @returns                   FPGA_OK on success
 */
fpga_result bmcGetLastPowerdownCause(fpga_token token, char **cause);

/**
 * Return BMC reported last reset cause.
 *
 * @param[in] token           fpga_token object for device (FPGA_DEVICE type)
 * @param[out] cause          String describing last reset cause
 * @returns                   FPGA_OK on success
 */
fpga_result bmcGetLastResetCause(fpga_token token, char **cause);

/**
 * Set BMC threshold values for a sensor.
 *
 * @param[in] sdr_h    bmc_sdr_handle
 * @param[in] sensor   The sensor of interest
 * @param[in] thresh   A pointer to a list of thresholds to set
 * @returns            FPGA_OK on success
 */
fpga_result bmcSetHWThresholds(bmc_sdr_handle sdr_h, uint32_t sensor,
			       threshold_list *thresh);


#ifdef __cplusplus
}
#endif

#endif /* !BMC_H */
