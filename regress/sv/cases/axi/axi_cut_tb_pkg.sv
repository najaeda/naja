// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

// Standalone AXI type package for tb_axi_cut_smoke.
//
// Manually expands AXI_TYPEDEF_ALL(axi, addr_t=32b, id_t=8b, data_t=64b,
// strb_t=8b, user_t=1b) with plain logic widths so the package compiles
// in the Verilator sim context without needing axi/typedef.svh or axi_pkg.
//
// Must match axi_cut_top_pkg exactly (same field names and packed order).

package axi_cut_tb_pkg;

  // ------------------------------------------------------------------ //
  // Base types                                                           //
  // ------------------------------------------------------------------ //
  typedef logic  [7:0] id_t;
  typedef logic [31:0] addr_t;
  typedef logic [63:0] data_t;
  typedef logic  [7:0] strb_t;
  typedef logic  [0:0] user_t;

  // AXI4 field widths (mirrors axi_pkg)
  typedef logic  [7:0] len_t;     // burst length − 1
  typedef logic  [2:0] size_t;    // log2(bytes per beat)
  typedef logic  [1:0] burst_t;   // FIXED/INCR/WRAP
  typedef logic  [3:0] cache_t;
  typedef logic  [2:0] prot_t;
  typedef logic  [3:0] qos_t;
  typedef logic  [3:0] region_t;
  typedef logic  [5:0] atop_t;    // AXI5 atomic extension

  // ------------------------------------------------------------------ //
  // Channel structs — field order must be identical to the macros        //
  // ------------------------------------------------------------------ //

  // AW: 8+32+8+3+2+1+4+3+4+4+6+1 = 76 bits
  typedef struct packed {
    id_t     id;
    addr_t   addr;
    len_t    len;
    size_t   size;
    burst_t  burst;
    logic    lock;
    cache_t  cache;
    prot_t   prot;
    qos_t    qos;
    region_t region;
    atop_t   atop;
    user_t   user;
  } aw_chan_t;

  // W: 64+8+1+1 = 74 bits
  typedef struct packed {
    data_t data;
    strb_t strb;
    logic  last;
    user_t user;
  } w_chan_t;

  // B: 8+2+1 = 11 bits
  typedef struct packed {
    id_t   id;
    logic [1:0] resp;
    user_t user;
  } b_chan_t;

  // AR: 8+32+8+3+2+1+4+3+4+4+1 = 70 bits  (no atop)
  typedef struct packed {
    id_t     id;
    addr_t   addr;
    len_t    len;
    size_t   size;
    burst_t  burst;
    logic    lock;
    cache_t  cache;
    prot_t   prot;
    qos_t    qos;
    region_t region;
    user_t   user;
  } ar_chan_t;

  // R: 8+64+2+1+1 = 76 bits
  typedef struct packed {
    id_t   id;
    data_t data;
    logic [1:0] resp;
    logic  last;
    user_t user;
  } r_chan_t;

  // ------------------------------------------------------------------ //
  // Request / response structs                                           //
  // ------------------------------------------------------------------ //

  // req_t: 76+1+74+1+1+70+1+1 = 225 bits
  typedef struct packed {
    aw_chan_t aw;
    logic     aw_valid;
    w_chan_t  w;
    logic     w_valid;
    logic     b_ready;
    ar_chan_t ar;
    logic     ar_valid;
    logic     r_ready;
  } req_t;

  // resp_t: 1+1+1+1+11+1+76 = 92 bits
  typedef struct packed {
    logic    aw_ready;
    logic    ar_ready;
    logic    w_ready;
    logic    b_valid;
    b_chan_t b;
    logic    r_valid;
    r_chan_t r;
  } resp_t;

endpackage
