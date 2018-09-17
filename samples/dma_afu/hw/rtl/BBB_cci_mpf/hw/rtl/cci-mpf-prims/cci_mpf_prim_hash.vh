//
// Copyright (c) 2015, Intel Corporation
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

`ifndef CCI_MPF_PRIM_HASH_VH
`define CCI_MPF_PRIM_HASH_VH

//
// 32 bit hash function.
//

function automatic [31:0] hash32;
    input [31:0] d;

    //
    // CRC-32 (IEEE802.3), polynomial 0 1 2 4 5 7 8 10 11 12 16 22 23 26 32.
    //
    logic [31:0] hash;
    hash[0] = d[31] ^ d[30] ^ d[29] ^ d[28] ^ d[26] ^ d[25] ^ d[24] ^ 
              d[16] ^ d[12] ^ d[10] ^ d[9] ^ d[6] ^ d[0];
    hash[1] = d[28] ^ d[27] ^ d[24] ^ d[17] ^ d[16] ^ d[13] ^ d[12] ^ 
              d[11] ^ d[9] ^ d[7] ^ d[6] ^ d[1] ^ d[0];
    hash[2] = d[31] ^ d[30] ^ d[26] ^ d[24] ^ d[18] ^ d[17] ^ d[16] ^ 
              d[14] ^ d[13] ^ d[9] ^ d[8] ^ d[7] ^ d[6] ^ d[2] ^ 
              d[1] ^ d[0];
    hash[3] = d[31] ^ d[27] ^ d[25] ^ d[19] ^ d[18] ^ d[17] ^ d[15] ^ 
              d[14] ^ d[10] ^ d[9] ^ d[8] ^ d[7] ^ d[3] ^ d[2] ^ 
              d[1];
    hash[4] = d[31] ^ d[30] ^ d[29] ^ d[25] ^ d[24] ^ d[20] ^ d[19] ^ 
              d[18] ^ d[15] ^ d[12] ^ d[11] ^ d[8] ^ d[6] ^ d[4] ^ 
              d[3] ^ d[2] ^ d[0];
    hash[5] = d[29] ^ d[28] ^ d[24] ^ d[21] ^ d[20] ^ d[19] ^ d[13] ^ 
              d[10] ^ d[7] ^ d[6] ^ d[5] ^ d[4] ^ d[3] ^ d[1] ^ d[0];
    hash[6] = d[30] ^ d[29] ^ d[25] ^ d[22] ^ d[21] ^ d[20] ^ d[14] ^ 
              d[11] ^ d[8] ^ d[7] ^ d[6] ^ d[5] ^ d[4] ^ d[2] ^ d[1];
    hash[7] = d[29] ^ d[28] ^ d[25] ^ d[24] ^ d[23] ^ d[22] ^ d[21] ^ 
              d[16] ^ d[15] ^ d[10] ^ d[8] ^ d[7] ^ d[5] ^ d[3] ^ 
              d[2] ^ d[0];
    hash[8] = d[31] ^ d[28] ^ d[23] ^ d[22] ^ d[17] ^ d[12] ^ d[11] ^ 
              d[10] ^ d[8] ^ d[4] ^ d[3] ^ d[1] ^ d[0];
    hash[9] = d[29] ^ d[24] ^ d[23] ^ d[18] ^ d[13] ^ d[12] ^ d[11] ^ 
              d[9] ^ d[5] ^ d[4] ^ d[2] ^ d[1];
    hash[10] = d[31] ^ d[29] ^ d[28] ^ d[26] ^ d[19] ^ d[16] ^ d[14] ^ 
               d[13] ^ d[9] ^ d[5] ^ d[3] ^ d[2] ^ d[0];
    hash[11] = d[31] ^ d[28] ^ d[27] ^ d[26] ^ d[25] ^ d[24] ^ d[20] ^ 
               d[17] ^ d[16] ^ d[15] ^ d[14] ^ d[12] ^ d[9] ^ d[4] ^ 
               d[3] ^ d[1] ^ d[0];
    hash[12] = d[31] ^ d[30] ^ d[27] ^ d[24] ^ d[21] ^ d[18] ^ d[17] ^ 
               d[15] ^ d[13] ^ d[12] ^ d[9] ^ d[6] ^ d[5] ^ d[4] ^ 
               d[2] ^ d[1] ^ d[0];
    hash[13] = d[31] ^ d[28] ^ d[25] ^ d[22] ^ d[19] ^ d[18] ^ d[16] ^ 
               d[14] ^ d[13] ^ d[10] ^ d[7] ^ d[6] ^ d[5] ^ d[3] ^ 
               d[2] ^ d[1];
    hash[14] = d[29] ^ d[26] ^ d[23] ^ d[20] ^ d[19] ^ d[17] ^ d[15] ^ 
               d[14] ^ d[11] ^ d[8] ^ d[7] ^ d[6] ^ d[4] ^ d[3] ^ 
               d[2];
    hash[15] = d[30] ^ d[27] ^ d[24] ^ d[21] ^ d[20] ^ d[18] ^ d[16] ^ 
               d[15] ^ d[12] ^ d[9] ^ d[8] ^ d[7] ^ d[5] ^ d[4] ^ 
               d[3];
    hash[16] = d[30] ^ d[29] ^ d[26] ^ d[24] ^ d[22] ^ d[21] ^ d[19] ^ 
               d[17] ^ d[13] ^ d[12] ^ d[8] ^ d[5] ^ d[4] ^ d[0];
    hash[17] = d[31] ^ d[30] ^ d[27] ^ d[25] ^ d[23] ^ d[22] ^ d[20] ^ 
               d[18] ^ d[14] ^ d[13] ^ d[9] ^ d[6] ^ d[5] ^ d[1];
    hash[18] = d[31] ^ d[28] ^ d[26] ^ d[24] ^ d[23] ^ d[21] ^ d[19] ^ 
               d[15] ^ d[14] ^ d[10] ^ d[7] ^ d[6] ^ d[2];
    hash[19] = d[29] ^ d[27] ^ d[25] ^ d[24] ^ d[22] ^ d[20] ^ d[16] ^ 
               d[15] ^ d[11] ^ d[8] ^ d[7] ^ d[3];
    hash[20] = d[30] ^ d[28] ^ d[26] ^ d[25] ^ d[23] ^ d[21] ^ d[17] ^ 
               d[16] ^ d[12] ^ d[9] ^ d[8] ^ d[4];
    hash[21] = d[31] ^ d[29] ^ d[27] ^ d[26] ^ d[24] ^ d[22] ^ d[18] ^ 
               d[17] ^ d[13] ^ d[10] ^ d[9] ^ d[5];
    hash[22] = d[31] ^ d[29] ^ d[27] ^ d[26] ^ d[24] ^ d[23] ^ d[19] ^ 
               d[18] ^ d[16] ^ d[14] ^ d[12] ^ d[11] ^ d[9] ^ d[0];
    hash[23] = d[31] ^ d[29] ^ d[27] ^ d[26] ^ d[20] ^ d[19] ^ d[17] ^ 
               d[16] ^ d[15] ^ d[13] ^ d[9] ^ d[6] ^ d[1] ^ d[0];
    hash[24] = d[30] ^ d[28] ^ d[27] ^ d[21] ^ d[20] ^ d[18] ^ d[17] ^ 
               d[16] ^ d[14] ^ d[10] ^ d[7] ^ d[2] ^ d[1];
    hash[25] = d[31] ^ d[29] ^ d[28] ^ d[22] ^ d[21] ^ d[19] ^ d[18] ^ 
               d[17] ^ d[15] ^ d[11] ^ d[8] ^ d[3] ^ d[2];
    hash[26] = d[31] ^ d[28] ^ d[26] ^ d[25] ^ d[24] ^ d[23] ^ d[22] ^ 
               d[20] ^ d[19] ^ d[18] ^ d[10] ^ d[6] ^ d[4] ^ d[3] ^ 
               d[0];
    hash[27] = d[29] ^ d[27] ^ d[26] ^ d[25] ^ d[24] ^ d[23] ^ d[21] ^ 
               d[20] ^ d[19] ^ d[11] ^ d[7] ^ d[5] ^ d[4] ^ d[1];
    hash[28] = d[30] ^ d[28] ^ d[27] ^ d[26] ^ d[25] ^ d[24] ^ d[22] ^ 
               d[21] ^ d[20] ^ d[12] ^ d[8] ^ d[6] ^ d[5] ^ d[2];
    hash[29] = d[31] ^ d[29] ^ d[28] ^ d[27] ^ d[26] ^ d[25] ^ d[23] ^ 
               d[22] ^ d[21] ^ d[13] ^ d[9] ^ d[7] ^ d[6] ^ d[3];
    hash[30] = d[30] ^ d[29] ^ d[28] ^ d[27] ^ d[26] ^ d[24] ^ d[23] ^ 
               d[22] ^ d[14] ^ d[10] ^ d[8] ^ d[7] ^ d[4];
    hash[31] = d[31] ^ d[30] ^ d[29] ^ d[28] ^ d[27] ^ d[25] ^ d[24] ^ 
               d[23] ^ d[15] ^ d[11] ^ d[9] ^ d[8] ^ d[5];

    hash32 = hash;
endfunction

function automatic [31:0] hash32_inv;
    input [31:0] d;

    //
    // Inverse of hash32
    //
    logic [31:0] hash;
    hash[0] = d[31] ^ d[29] ^ d[27] ^ d[25] ^ d[23] ^ d[21] ^ d[20] ^
              d[16] ^ d[14] ^ d[9] ^ d[5] ^ d[2] ^ d[1];
    hash[1] = d[31] ^ d[30] ^ d[29] ^ d[28] ^ d[27] ^ d[26] ^ d[25] ^
              d[24] ^ d[23] ^ d[22] ^ d[20] ^ d[17] ^ d[16] ^ d[15] ^
              d[14] ^ d[10] ^ d[9] ^ d[6] ^ d[5] ^ d[3] ^ d[1] ^
              d[0];
    hash[2] = d[30] ^ d[28] ^ d[26] ^ d[24] ^ d[20] ^ d[18] ^ d[17] ^
              d[15] ^ d[14] ^ d[11] ^ d[10] ^ d[9] ^ d[7] ^ d[6] ^
              d[5] ^ d[4];
    hash[3] = d[31] ^ d[29] ^ d[27] ^ d[25] ^ d[21] ^ d[19] ^ d[18] ^
              d[16] ^ d[15] ^ d[12] ^ d[11] ^ d[10] ^ d[8] ^ d[7] ^
              d[6] ^ d[5] ^ d[0];
    hash[4] = d[31] ^ d[30] ^ d[29] ^ d[28] ^ d[27] ^ d[26] ^ d[25] ^
              d[23] ^ d[22] ^ d[21] ^ d[19] ^ d[17] ^ d[14] ^ d[13] ^
              d[12] ^ d[11] ^ d[8] ^ d[7] ^ d[6] ^ d[5] ^ d[2] ^
              d[0];
    hash[5] = d[30] ^ d[28] ^ d[26] ^ d[25] ^ d[24] ^ d[22] ^ d[21] ^
              d[18] ^ d[16] ^ d[15] ^ d[13] ^ d[12] ^ d[8] ^ d[7] ^
              d[6] ^ d[5] ^ d[3] ^ d[2];
    hash[6] = d[31] ^ d[29] ^ d[27] ^ d[26] ^ d[25] ^ d[23] ^ d[22] ^
              d[19] ^ d[17] ^ d[16] ^ d[14] ^ d[13] ^ d[9] ^ d[8] ^
              d[7] ^ d[6] ^ d[4] ^ d[3] ^ d[0];
    hash[7] = d[31] ^ d[30] ^ d[29] ^ d[28] ^ d[26] ^ d[25] ^ d[24] ^
              d[21] ^ d[18] ^ d[17] ^ d[16] ^ d[15] ^ d[10] ^ d[8] ^
              d[7] ^ d[4] ^ d[2] ^ d[0];
    hash[8] = d[30] ^ d[26] ^ d[23] ^ d[22] ^ d[21] ^ d[20] ^ d[19] ^
              d[18] ^ d[17] ^ d[14] ^ d[11] ^ d[8] ^ d[3] ^ d[2];
    hash[9] = d[31] ^ d[27] ^ d[24] ^ d[23] ^ d[22] ^ d[21] ^ d[20] ^
              d[19] ^ d[18] ^ d[15] ^ d[12] ^ d[9] ^ d[4] ^ d[3];
    hash[10] = d[31] ^ d[29] ^ d[28] ^ d[27] ^ d[24] ^ d[22] ^ d[19] ^
               d[14] ^ d[13] ^ d[10] ^ d[9] ^ d[4] ^ d[2] ^ d[1] ^
               d[0];
    hash[11] = d[31] ^ d[30] ^ d[28] ^ d[27] ^ d[21] ^ d[16] ^ d[15] ^
               d[11] ^ d[10] ^ d[9] ^ d[3] ^ d[0];
    hash[12] = d[28] ^ d[27] ^ d[25] ^ d[23] ^ d[22] ^ d[21] ^ d[20] ^
               d[17] ^ d[14] ^ d[12] ^ d[11] ^ d[10] ^ d[9] ^ d[5] ^
               d[4] ^ d[2];
    hash[13] = d[29] ^ d[28] ^ d[26] ^ d[24] ^ d[23] ^ d[22] ^ d[21] ^
               d[18] ^ d[15] ^ d[13] ^ d[12] ^ d[11] ^ d[10] ^ d[6] ^
               d[5] ^ d[3] ^ d[0];
    hash[14] = d[30] ^ d[29] ^ d[27] ^ d[25] ^ d[24] ^ d[23] ^ d[22] ^
               d[19] ^ d[16] ^ d[14] ^ d[13] ^ d[12] ^ d[11] ^ d[7] ^
               d[6] ^ d[4] ^ d[1];
    hash[15] = d[31] ^ d[30] ^ d[28] ^ d[26] ^ d[25] ^ d[24] ^ d[23] ^
               d[20] ^ d[17] ^ d[15] ^ d[14] ^ d[13] ^ d[12] ^ d[8] ^
               d[7] ^ d[5] ^ d[2] ^ d[0];
    hash[16] = d[26] ^ d[24] ^ d[23] ^ d[20] ^ d[18] ^ d[15] ^ d[13] ^
               d[8] ^ d[6] ^ d[5] ^ d[3] ^ d[2] ^ d[0];
    hash[17] = d[27] ^ d[25] ^ d[24] ^ d[21] ^ d[19] ^ d[16] ^ d[14] ^
               d[9] ^ d[7] ^ d[6] ^ d[4] ^ d[3] ^ d[1];
    hash[18] = d[28] ^ d[26] ^ d[25] ^ d[22] ^ d[20] ^ d[17] ^ d[15] ^
               d[10] ^ d[8] ^ d[7] ^ d[5] ^ d[4] ^ d[2];
    hash[19] = d[29] ^ d[27] ^ d[26] ^ d[23] ^ d[21] ^ d[18] ^ d[16] ^
               d[11] ^ d[9] ^ d[8] ^ d[6] ^ d[5] ^ d[3];
    hash[20] = d[30] ^ d[28] ^ d[27] ^ d[24] ^ d[22] ^ d[19] ^ d[17] ^
               d[12] ^ d[10] ^ d[9] ^ d[7] ^ d[6] ^ d[4] ^ d[0];
    hash[21] = d[31] ^ d[29] ^ d[28] ^ d[25] ^ d[23] ^ d[20] ^ d[18] ^
               d[13] ^ d[11] ^ d[10] ^ d[8] ^ d[7] ^ d[5] ^ d[1] ^
               d[0];
    hash[22] = d[31] ^ d[30] ^ d[27] ^ d[26] ^ d[25] ^ d[24] ^ d[23] ^
               d[20] ^ d[19] ^ d[16] ^ d[12] ^ d[11] ^ d[8] ^ d[6] ^
               d[5] ^ d[0];
    hash[23] = d[29] ^ d[28] ^ d[26] ^ d[24] ^ d[23] ^ d[17] ^ d[16] ^
               d[14] ^ d[13] ^ d[12] ^ d[7] ^ d[6] ^ d[5] ^ d[2] ^
               d[0];
    hash[24] = d[30] ^ d[29] ^ d[27] ^ d[25] ^ d[24] ^ d[18] ^ d[17] ^
               d[15] ^ d[14] ^ d[13] ^ d[8] ^ d[7] ^ d[6] ^ d[3] ^
               d[1] ^ d[0];
    hash[25] = d[31] ^ d[30] ^ d[28] ^ d[26] ^ d[25] ^ d[19] ^ d[18] ^
               d[16] ^ d[15] ^ d[14] ^ d[9] ^ d[8] ^ d[7] ^ d[4] ^
               d[2] ^ d[1] ^ d[0];
    hash[26] = d[26] ^ d[25] ^ d[23] ^ d[21] ^ d[19] ^ d[17] ^ d[15] ^
               d[14] ^ d[10] ^ d[8] ^ d[3];
    hash[27] = d[27] ^ d[26] ^ d[24] ^ d[22] ^ d[20] ^ d[18] ^ d[16] ^
               d[15] ^ d[11] ^ d[9] ^ d[4] ^ d[0];
    hash[28] = d[28] ^ d[27] ^ d[25] ^ d[23] ^ d[21] ^ d[19] ^ d[17] ^
               d[16] ^ d[12] ^ d[10] ^ d[5] ^ d[1];
    hash[29] = d[29] ^ d[28] ^ d[26] ^ d[24] ^ d[22] ^ d[20] ^ d[18] ^
               d[17] ^ d[13] ^ d[11] ^ d[6] ^ d[2];
    hash[30] = d[30] ^ d[29] ^ d[27] ^ d[25] ^ d[23] ^ d[21] ^ d[19] ^
               d[18] ^ d[14] ^ d[12] ^ d[7] ^ d[3] ^ d[0];
    hash[31] = d[31] ^ d[30] ^ d[28] ^ d[26] ^ d[24] ^ d[22] ^ d[20] ^
               d[19] ^ d[15] ^ d[13] ^ d[8] ^ d[4] ^ d[1] ^ d[0];

    hash32_inv = hash;
endfunction

`endif //  `ifndef CCI_MPF_PRIM_HASH_VH
