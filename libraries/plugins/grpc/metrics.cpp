// Copyright(c) 2023, Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <opae/log.h>
#include <opae/types.h>

#include "grpc_client.hpp"
#include "mock/opae_std.h"
#include "remote.h"

fpga_result __REMOTE_API__ remote_fpgaGetNumMetrics(fpga_handle handle,
                                                    uint64_t *num_metrics) {
  _remote_handle *h;
  OPAEClient *client;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!num_metrics) {
    OPAE_ERR("NULL num_metrics pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  return client->fpgaGetNumMetrics(h->hdr.handle_id, *num_metrics);
}

fpga_result __REMOTE_API__ remote_fpgaGetMetricsInfo(
    fpga_handle handle, fpga_metric_info *metric_info, uint64_t *num_metrics) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!metric_info) {
    OPAE_ERR("NULL metric_info pointer");
    return FPGA_INVALID_PARAM;
  }

  if (!num_metrics) {
    OPAE_ERR("NULL num_metrics pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);
  std::vector<fpga_metric_info> info;

  res = client->fpgaGetMetricsInfo(h->hdr.handle_id, *num_metrics, info);

  if (res == FPGA_OK) {
    size_t count = std::min((size_t)*num_metrics, info.size());
    std::copy(info.cbegin(), info.cbegin() + count, metric_info);
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaGetMetricsByIndex(
    fpga_handle handle, uint64_t *metric_num, uint64_t num_metric_indexes,
    fpga_metric *metrics) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!metric_num) {
    OPAE_ERR("NULL metric_num pointer");
    return FPGA_INVALID_PARAM;
  }

  if (!metrics) {
    OPAE_ERR("NULL metrics pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  std::vector<uint64_t> vmetric_num;
  vmetric_num.reserve(num_metric_indexes);
  vmetric_num.resize(num_metric_indexes);
  std::copy(metric_num, metric_num + num_metric_indexes, vmetric_num.begin());

  std::vector<fpga_metric> vmetrics;

  res = client->fpgaGetMetricsByIndex(h->hdr.handle_id, vmetric_num,
                                      num_metric_indexes, vmetrics);

  if (res == FPGA_OK)
    std::copy(vmetrics.cbegin(), vmetrics.cbegin() + num_metric_indexes,
              metrics);

  return res;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsByName(fpga_handle handle, char **metrics_names,
                            uint64_t num_metric_names, fpga_metric *metrics) {
  _remote_handle *h;
  OPAEClient *client;
  uint64_t i;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!metrics_names) {
    OPAE_ERR("NULL metrics_names pointer");
    return FPGA_INVALID_PARAM;
  }

  if (!metrics) {
    OPAE_ERR("NULL metrics pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  std::vector<std::string> vmetrics_names;
  vmetrics_names.reserve(num_metric_names);
  for (i = 0; i < num_metric_names; ++i)
    vmetrics_names.push_back(std::string(metrics_names[i]));

  std::vector<fpga_metric> vmetrics;

  res = client->fpgaGetMetricsByName(h->hdr.handle_id, vmetrics_names,
                                     num_metric_names, vmetrics);

  if (res == FPGA_OK)
    std::copy(vmetrics.cbegin(), vmetrics.cbegin() + num_metric_names, metrics);

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaGetMetricsThresholdInfo(
    fpga_handle handle, metric_threshold *metric_thresholds,
    uint32_t *num_thresholds) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!metric_thresholds) {
    OPAE_ERR("NULL metric_thresholds pointer");
    return FPGA_INVALID_PARAM;
  }

  if (!num_thresholds) {
    OPAE_ERR("NULL num_thresholds pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  std::vector<metric_threshold> vmetric_thresholds;

  res = client->fpgaGetMetricsThresholdInfo(h->hdr.handle_id, *num_thresholds,
                                            vmetric_thresholds);

  if (res == FPGA_OK) {
    size_t count = std::min((size_t)*num_thresholds, vmetric_thresholds.size());
    std::copy(vmetric_thresholds.cbegin(), vmetric_thresholds.cbegin() + count,
              metric_thresholds);
  }

  return res;
}
