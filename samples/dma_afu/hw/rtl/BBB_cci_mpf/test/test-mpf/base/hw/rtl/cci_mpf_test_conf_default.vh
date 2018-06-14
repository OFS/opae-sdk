//
// Copyright (c) 2016, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Application default configuration can override the base configuration.
`include "cci_mpf_test_conf.vh"


`ifndef AFU_CLOCK_FREQ
  `define AFU_CLOCK_FREQ 400
`endif

//
// MPF default configuration.
//
`ifndef MPF_CONF_SORT_READ_RESPONSES
  `define MPF_CONF_SORT_READ_RESPONSES 0
`endif

`ifndef MPF_CONF_PRESERVE_WRITE_MDATA
  `define MPF_CONF_PRESERVE_WRITE_MDATA 0
`endif

`ifndef MPF_CONF_ENABLE_VTP
  `define MPF_CONF_ENABLE_VTP 1
`endif

`ifndef MPF_CONF_ENABLE_VC_MAP
  `define MPF_CONF_ENABLE_VC_MAP 0
`endif

`ifndef MPF_CONF_ENABLE_DYNAMIC_VC_MAPPING
  // This flag only matters when MPF_CONF_ENABLE_VC_MAP is 1
  `define MPF_CONF_ENABLE_DYNAMIC_VC_MAPPING 1
`endif

`ifndef MPF_CONF_ENABLE_LATENCY_QOS
  `define MPF_CONF_ENABLE_LATENCY_QOS 0
`endif

`ifndef MPF_CONF_ENFORCE_WR_ORDER
  `define MPF_CONF_ENFORCE_WR_ORDER 0
`endif

`ifndef MPF_CONF_ENABLE_PARTIAL_WRITES
  `define MPF_CONF_ENABLE_PARTIAL_WRITES 0
`endif

`ifndef MPF_CONF_MERGE_DUPLICATE_READS
  `define MPF_CONF_MERGE_DUPLICATE_READS 0
`endif
