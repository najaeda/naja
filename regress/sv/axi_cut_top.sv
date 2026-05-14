// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

// Concrete-type wrapper for axi_cut for the Naja SV regression.
//
// Uses a realistic AXI4+ATOP configuration:
//   address 32-bit, data 64-bit, ID 8-bit, user 1-bit
//
// Types are generated via the standard axi/typedef.svh macros so the
// resulting design matches a real SoC integration.  Each of the five
// spill registers inside axi_cut becomes a properly-sized flop stage:
//   AW  76 b  (id+addr+len+size+burst+lock+cache+prot+qos+region+atop+user)
//   W   74 b  (data+strb+last+user)
//   B   11 b  (id+resp+user)
//   AR  70 b  (id+addr+len+size+burst+lock+cache+prot+qos+region+user)
//   R   76 b  (id+data+resp+last+user)

package axi_cut_top_pkg;
  `include "axi/typedef.svh"

  typedef logic [31:0] addr_t;
  typedef logic [63:0] data_t;
  typedef logic  [7:0] strb_t;
  typedef logic  [7:0] id_t;
  typedef logic  [0:0] user_t;

  `AXI_TYPEDEF_ALL(axi, addr_t, id_t, data_t, strb_t, user_t)
endpackage

module axi_cut_top
  import axi_cut_top_pkg::*;
(
  input  logic      clk_i,
  input  logic      rst_ni,
  // slave port
  input  axi_req_t  slv_req_i,
  output axi_resp_t slv_resp_o,
  // master port
  output axi_req_t  mst_req_o,
  input  axi_resp_t mst_resp_i
);
  axi_cut #(
    .aw_chan_t  (axi_aw_chan_t),
    .w_chan_t   (axi_w_chan_t),
    .b_chan_t   (axi_b_chan_t),
    .ar_chan_t  (axi_ar_chan_t),
    .r_chan_t   (axi_r_chan_t),
    .axi_req_t  (axi_req_t),
    .axi_resp_t (axi_resp_t)
  ) i_cut (
    .clk_i     (clk_i),
    .rst_ni    (rst_ni),
    .slv_req_i (slv_req_i),
    .slv_resp_o(slv_resp_o),
    .mst_req_o (mst_req_o),
    .mst_resp_i(mst_resp_i)
  );
endmodule
