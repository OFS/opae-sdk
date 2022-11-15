// Copyright(c) 2022, Intel Corporation
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
#endif // HAVE_CONFIG_H

#include <opae/types.h>
#include <opae/log.h>

#include "mock/opae_std.h"

#include "remote.h"
#include "request.h"
#include "response.h"

fpga_result __REMOTE_API__
remote_fpgaGetNumMetrics(fpga_handle handle, uint64_t *num_metrics)
{
	opae_fpgaGetNumMetrics_request req;
	opae_fpgaGetNumMetrics_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (!num_metrics) {
		OPAE_ERR("NULL num_metrics pointer");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;

	tok = h->token;

	req.handle = h->hdr;

	req_json = opae_encode_fpgaGetNumMetrics_request_36(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaGetNumMetrics_response_36(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*num_metrics = resp.num_metrics;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsInfo(fpga_handle handle,
			  fpga_metric_info *metric_info,
			  uint64_t *num_metrics)
{
	opae_fpgaGetMetricsInfo_request req;
	opae_fpgaGetMetricsInfo_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

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

	h = (struct _remote_handle *)handle;

	tok = h->token;

	req.handle = h->hdr;
	req.num_metrics = *num_metrics;

	req_json = opae_encode_fpgaGetMetricsInfo_request_37(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaGetMetricsInfo_response_37(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		memcpy(metric_info,
		       resp.info,
		       resp.num_metrics * sizeof(fpga_metric_info));

		opae_free(resp.info);

		*num_metrics = resp.num_metrics;
	}

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsByIndex(fpga_handle handle,
			     uint64_t *metric_num,
			     uint64_t num_metric_indexes,
			     fpga_metric *metrics)
{
	opae_fpgaGetMetricsByIndex_request req;
	opae_fpgaGetMetricsByIndex_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

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

	h = (struct _remote_handle *)handle;

	tok = h->token;

	req.handle = h->hdr;
	req.metric_num = metric_num;
	req.num_metric_indexes = num_metric_indexes;
	resp.num_metric_indexes = num_metric_indexes;

	req_json = opae_encode_fpgaGetMetricsByIndex_request_38(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaGetMetricsByIndex_response_38(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		memcpy(metrics,
		       resp.metrics,
		       resp.num_metric_indexes * sizeof(fpga_metric));

		opae_free(resp.metrics);
	}

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsByName(fpga_handle handle,
			    char **metrics_names,
			    uint64_t num_metric_names,
			    fpga_metric *metrics)
{
	opae_fpgaGetMetricsByName_request req;
	opae_fpgaGetMetricsByName_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

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

	h = (struct _remote_handle *)handle;

	tok = h->token;

	req.handle = h->hdr;
	req.metrics_names = metrics_names;
	req.num_metric_names = num_metric_names;
	resp.num_metric_names = num_metric_names;

	req_json = opae_encode_fpgaGetMetricsByName_request_39(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaGetMetricsByName_response_39(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		memcpy(metrics,
		       resp.metrics,
		       resp.num_metric_names * sizeof(fpga_metric));

		opae_free(resp.metrics);
	}

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsThresholdInfo(fpga_handle handle,
				   metric_threshold *metric_thresholds,
				   uint32_t *num_thresholds)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) metric_thresholds;
(void) num_thresholds;


	return result;
}
