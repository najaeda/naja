// SPDX-FileCopyrightText: lowRISC contributors (OpenTitan project)
// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
//
// Standalone excerpt of the OpenTitan AES Canright S-box and its packages.

package aes_pkg;
  typedef enum logic [1:0] { CIPH_FWD = 2'b01, CIPH_INV = 2'b10 } ciph_op_e;
function automatic logic [7:0] aes_mvm(
  logic [7:0] vec_b,
  logic [7:0] mat_a [8]
);
  logic [7:0] vec_c;
  vec_c = '0;
  for (int i = 0; i < 8; i++) begin
    for (int j = 0; j < 8; j++) begin
      vec_c[i] = vec_c[i] ^ (mat_a[j][i] & vec_b[7-j]);
    end
  end
  return vec_c;
endfunction
endpackage

// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// AES Canright SBox package
//
// For details, see the following documents:
// - Canright, "A very compact Rijndael S-box", technical report
//   available at https://hdl.handle.net/10945/25608
// - Canright, "A very compact 'perfectly masked' S-box for AES (corrected)", paper
//   available at https://eprint.iacr.org/2009/011.pdf

package aes_sbox_canright_pkg;

  // Multiplication in GF(2^2), using normal basis [Omega^2, Omega]
  // (see Figure 14 in the technical report)
  function automatic logic [1:0] aes_mul_gf2p2(logic [1:0] g, logic [1:0] d);
    logic [1:0] f;
    logic       a, b, c;
    a    = g[1] & d[1];
    b    = (^g) & (^d);
    c    = g[0] & d[0];
    f[1] = a ^ b;
    f[0] = c ^ b;
    return f;
  endfunction

  // Scale by Omega^2 = N in GF(2^2), using normal basis [Omega^2, Omega]
  // (see Figure 16 in the technical report)
  function automatic logic [1:0] aes_scale_omega2_gf2p2(logic [1:0] g);
    logic [1:0] d;
    d[1] = g[0];
    d[0] = g[1] ^ g[0];
    return d;
  endfunction

  // Scale by Omega = N^2 in GF(2^2), using normal basis [Omega^2, Omega]
  // (see Figure 15 in the technical report)
  function automatic logic [1:0] aes_scale_omega_gf2p2(logic [1:0] g);
    logic [1:0] d;
    d[1] = g[1] ^ g[0];
    d[0] = g[1];
    return d;
  endfunction

  // Square in GF(2^2), using normal basis [Omega^2, Omega]
  // (see Figures 8 and 10 in the technical report)
  function automatic logic [1:0] aes_square_gf2p2(logic [1:0] g);
    logic [1:0] d;
    d[1] = g[0];
    d[0] = g[1];
    return d;
  endfunction

  // Multiplication in GF(2^4), using normal basis [alpha^8, alpha^2]
  // (see Figure 13 in the technical report)
  function automatic logic [3:0] aes_mul_gf2p4(logic [3:0] gamma, logic [3:0] delta);
    logic [3:0] theta;
    logic [1:0] a, b, c;
    a          = aes_mul_gf2p2(gamma[3:2], delta[3:2]);
    b          = aes_mul_gf2p2(gamma[3:2] ^ gamma[1:0], delta[3:2] ^ delta[1:0]);
    c          = aes_mul_gf2p2(gamma[1:0], delta[1:0]);
    theta[3:2] = a ^ aes_scale_omega2_gf2p2(b);
    theta[1:0] = c ^ aes_scale_omega2_gf2p2(b);
    return theta;
  endfunction

  // Square and scale by nu in GF(2^4)/GF(2^2), using normal basis [alpha^8, alpha^2]
  // (see Figure 19 as well as Appendix A of the technical report)
  function automatic logic [3:0] aes_square_scale_gf2p4_gf2p2(logic [3:0] gamma);
    logic [3:0] delta;
    logic [1:0] a, b;
    a          = gamma[3:2] ^ gamma[1:0];
    b          = aes_square_gf2p2(gamma[1:0]);
    delta[3:2] = aes_square_gf2p2(a);
    delta[1:0] = aes_scale_omega_gf2p2(b);
    return delta;
  endfunction

  // Basis conversion matrices to convert between polynomial basis A, normal basis X
  // and basis S incorporating the bit matrix of the SBox. More specifically,
  // multiplication by X2X performs the transformation from normal basis X into
  // polynomial basis A, followed by the affine transformation (substep 2). Likewise,
  // multiplication by S2X performs the inverse affine transformation followed by the
  // transformation from polynomial basis A to normal basis X.
  // (see Appendix A of the technical report)
  parameter logic [7:0] A2X [8] = '{8'h98, 8'hf3, 8'hf2, 8'h48, 8'h09, 8'h81, 8'ha9, 8'hff};
  parameter logic [7:0] X2A [8] = '{8'h64, 8'h78, 8'h6e, 8'h8c, 8'h68, 8'h29, 8'hde, 8'h60};
  parameter logic [7:0] X2S [8] = '{8'h58, 8'h2d, 8'h9e, 8'h0b, 8'hdc, 8'h04, 8'h03, 8'h24};
  parameter logic [7:0] S2X [8] = '{8'h8c, 8'h79, 8'h05, 8'heb, 8'h12, 8'h04, 8'h51, 8'h53};

endpackage

// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// AES Canright SBox #4
//
// For details, see the technical report: Canright, "A very compact Rijndael S-box"
// available at https://hdl.handle.net/10945/25608

module aes_sbox_canright (
  input  aes_pkg::ciph_op_e op_i,
  input  logic [7:0]        data_i,
  output logic [7:0]        data_o
);

  import aes_pkg::*;
  import aes_sbox_canright_pkg::*;

  ///////////////
  // Functions //
  ///////////////

  // Inverse in GF(2^4), using normal basis [alpha^8, alpha^2]
  // (see Figure 12 in the technical report)
  function automatic logic [3:0] aes_inverse_gf2p4(logic [3:0] gamma);
    logic [3:0] delta;
    logic [1:0] a, b, c, d;
    a          = gamma[3:2] ^ gamma[1:0];
    b          = aes_mul_gf2p2(gamma[3:2], gamma[1:0]);
    c          = aes_scale_omega2_gf2p2(aes_square_gf2p2(a));
    d          = aes_square_gf2p2(c ^ b);
    delta[3:2] = aes_mul_gf2p2(d, gamma[1:0]);
    delta[1:0] = aes_mul_gf2p2(d, gamma[3:2]);
    return delta;
  endfunction

  // Inverse in GF(2^8), using normal basis [d^16, d]
  // (see Figure 11 in the technical report)
  function automatic logic [7:0] aes_inverse_gf2p8(logic [7:0] gamma);
    logic [7:0] delta;
    logic [3:0] a, b, c, d;
    a          = gamma[7:4] ^ gamma[3:0];
    b          = aes_mul_gf2p4(gamma[7:4], gamma[3:0]);
    c          = aes_square_scale_gf2p4_gf2p2(a);
    d          = aes_inverse_gf2p4(c ^ b);
    delta[7:4] = aes_mul_gf2p4(d, gamma[3:0]);
    delta[3:0] = aes_mul_gf2p4(d, gamma[7:4]);
    return delta;
  endfunction

  ///////////////////
  // Canright SBox //
  ///////////////////

  logic [7:0] data_basis_x, data_inverse;

  // Convert to normal basis X.
  assign data_basis_x = (op_i == CIPH_FWD) ? aes_mvm(data_i, A2X)         :
                        (op_i == CIPH_INV) ? aes_mvm(data_i ^ 8'h63, S2X) :
                                             aes_mvm(data_i, A2X);

  // Do the inversion in normal basis X.
  assign data_inverse = aes_inverse_gf2p8(data_basis_x);

  // Convert to basis S or A.
  assign data_o       = (op_i == CIPH_FWD) ? aes_mvm(data_inverse, X2S) ^ 8'h63 :
                        (op_i == CIPH_INV) ? aes_mvm(data_inverse, X2A) :
                                             aes_mvm(data_inverse, X2S) ^ 8'h63;

endmodule

// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// AES LUT-based SBox

module aes_sbox_lut (
  input  aes_pkg::ciph_op_e op_i,
  input  logic [7:0]        data_i,
  output logic [7:0]        data_o
);

  import aes_pkg::*;

  // Define the LUTs
  localparam logic [7:0] SBOX_FWD [256] = '{
    8'h63, 8'h7C, 8'h77, 8'h7B, 8'hF2, 8'h6B, 8'h6F, 8'hC5,
    8'h30, 8'h01, 8'h67, 8'h2B, 8'hFE, 8'hD7, 8'hAB, 8'h76,

    8'hCA, 8'h82, 8'hC9, 8'h7D, 8'hFA, 8'h59, 8'h47, 8'hF0,
    8'hAD, 8'hD4, 8'hA2, 8'hAF, 8'h9C, 8'hA4, 8'h72, 8'hC0,

    8'hB7, 8'hFD, 8'h93, 8'h26, 8'h36, 8'h3F, 8'hF7, 8'hCC,
    8'h34, 8'hA5, 8'hE5, 8'hF1, 8'h71, 8'hD8, 8'h31, 8'h15,

    8'h04, 8'hC7, 8'h23, 8'hC3, 8'h18, 8'h96, 8'h05, 8'h9A,
    8'h07, 8'h12, 8'h80, 8'hE2, 8'hEB, 8'h27, 8'hB2, 8'h75,

    8'h09, 8'h83, 8'h2C, 8'h1A, 8'h1B, 8'h6E, 8'h5A, 8'hA0,
    8'h52, 8'h3B, 8'hD6, 8'hB3, 8'h29, 8'hE3, 8'h2F, 8'h84,

    8'h53, 8'hD1, 8'h00, 8'hED, 8'h20, 8'hFC, 8'hB1, 8'h5B,
    8'h6A, 8'hCB, 8'hBE, 8'h39, 8'h4A, 8'h4C, 8'h58, 8'hCF,

    8'hD0, 8'hEF, 8'hAA, 8'hFB, 8'h43, 8'h4D, 8'h33, 8'h85,
    8'h45, 8'hF9, 8'h02, 8'h7F, 8'h50, 8'h3C, 8'h9F, 8'hA8,

    8'h51, 8'hA3, 8'h40, 8'h8F, 8'h92, 8'h9D, 8'h38, 8'hF5,
    8'hBC, 8'hB6, 8'hDA, 8'h21, 8'h10, 8'hFF, 8'hF3, 8'hD2,

    8'hCD, 8'h0C, 8'h13, 8'hEC, 8'h5F, 8'h97, 8'h44, 8'h17,
    8'hC4, 8'hA7, 8'h7E, 8'h3D, 8'h64, 8'h5D, 8'h19, 8'h73,

    8'h60, 8'h81, 8'h4F, 8'hDC, 8'h22, 8'h2A, 8'h90, 8'h88,
    8'h46, 8'hEE, 8'hB8, 8'h14, 8'hDE, 8'h5E, 8'h0B, 8'hDB,

    8'hE0, 8'h32, 8'h3A, 8'h0A, 8'h49, 8'h06, 8'h24, 8'h5C,
    8'hC2, 8'hD3, 8'hAC, 8'h62, 8'h91, 8'h95, 8'hE4, 8'h79,

    8'hE7, 8'hC8, 8'h37, 8'h6D, 8'h8D, 8'hD5, 8'h4E, 8'hA9,
    8'h6C, 8'h56, 8'hF4, 8'hEA, 8'h65, 8'h7A, 8'hAE, 8'h08,

    8'hBA, 8'h78, 8'h25, 8'h2E, 8'h1C, 8'hA6, 8'hB4, 8'hC6,
    8'hE8, 8'hDD, 8'h74, 8'h1F, 8'h4B, 8'hBD, 8'h8B, 8'h8A,

    8'h70, 8'h3E, 8'hB5, 8'h66, 8'h48, 8'h03, 8'hF6, 8'h0E,
    8'h61, 8'h35, 8'h57, 8'hB9, 8'h86, 8'hC1, 8'h1D, 8'h9E,

    8'hE1, 8'hF8, 8'h98, 8'h11, 8'h69, 8'hD9, 8'h8E, 8'h94,
    8'h9B, 8'h1E, 8'h87, 8'hE9, 8'hCE, 8'h55, 8'h28, 8'hDF,

    8'h8C, 8'hA1, 8'h89, 8'h0D, 8'hBF, 8'hE6, 8'h42, 8'h68,
    8'h41, 8'h99, 8'h2D, 8'h0F, 8'hB0, 8'h54, 8'hBB, 8'h16
  };

  localparam logic [7:0] SBOX_INV [256] = '{
    8'h52, 8'h09, 8'h6a, 8'hd5, 8'h30, 8'h36, 8'ha5, 8'h38,
    8'hbf, 8'h40, 8'ha3, 8'h9e, 8'h81, 8'hf3, 8'hd7, 8'hfb,

    8'h7c, 8'he3, 8'h39, 8'h82, 8'h9b, 8'h2f, 8'hff, 8'h87,
    8'h34, 8'h8e, 8'h43, 8'h44, 8'hc4, 8'hde, 8'he9, 8'hcb,

    8'h54, 8'h7b, 8'h94, 8'h32, 8'ha6, 8'hc2, 8'h23, 8'h3d,
    8'hee, 8'h4c, 8'h95, 8'h0b, 8'h42, 8'hfa, 8'hc3, 8'h4e,

    8'h08, 8'h2e, 8'ha1, 8'h66, 8'h28, 8'hd9, 8'h24, 8'hb2,
    8'h76, 8'h5b, 8'ha2, 8'h49, 8'h6d, 8'h8b, 8'hd1, 8'h25,

    8'h72, 8'hf8, 8'hf6, 8'h64, 8'h86, 8'h68, 8'h98, 8'h16,
    8'hd4, 8'ha4, 8'h5c, 8'hcc, 8'h5d, 8'h65, 8'hb6, 8'h92,

    8'h6c, 8'h70, 8'h48, 8'h50, 8'hfd, 8'hed, 8'hb9, 8'hda,
    8'h5e, 8'h15, 8'h46, 8'h57, 8'ha7, 8'h8d, 8'h9d, 8'h84,

    8'h90, 8'hd8, 8'hab, 8'h00, 8'h8c, 8'hbc, 8'hd3, 8'h0a,
    8'hf7, 8'he4, 8'h58, 8'h05, 8'hb8, 8'hb3, 8'h45, 8'h06,

    8'hd0, 8'h2c, 8'h1e, 8'h8f, 8'hca, 8'h3f, 8'h0f, 8'h02,
    8'hc1, 8'haf, 8'hbd, 8'h03, 8'h01, 8'h13, 8'h8a, 8'h6b,

    8'h3a, 8'h91, 8'h11, 8'h41, 8'h4f, 8'h67, 8'hdc, 8'hea,
    8'h97, 8'hf2, 8'hcf, 8'hce, 8'hf0, 8'hb4, 8'he6, 8'h73,

    8'h96, 8'hac, 8'h74, 8'h22, 8'he7, 8'had, 8'h35, 8'h85,
    8'he2, 8'hf9, 8'h37, 8'he8, 8'h1c, 8'h75, 8'hdf, 8'h6e,

    8'h47, 8'hf1, 8'h1a, 8'h71, 8'h1d, 8'h29, 8'hc5, 8'h89,
    8'h6f, 8'hb7, 8'h62, 8'h0e, 8'haa, 8'h18, 8'hbe, 8'h1b,

    8'hfc, 8'h56, 8'h3e, 8'h4b, 8'hc6, 8'hd2, 8'h79, 8'h20,
    8'h9a, 8'hdb, 8'hc0, 8'hfe, 8'h78, 8'hcd, 8'h5a, 8'hf4,

    8'h1f, 8'hdd, 8'ha8, 8'h33, 8'h88, 8'h07, 8'hc7, 8'h31,
    8'hb1, 8'h12, 8'h10, 8'h59, 8'h27, 8'h80, 8'hec, 8'h5f,

    8'h60, 8'h51, 8'h7f, 8'ha9, 8'h19, 8'hb5, 8'h4a, 8'h0d,
    8'h2d, 8'he5, 8'h7a, 8'h9f, 8'h93, 8'hc9, 8'h9c, 8'hef,

    8'ha0, 8'he0, 8'h3b, 8'h4d, 8'hae, 8'h2a, 8'hf5, 8'hb0,
    8'hc8, 8'heb, 8'hbb, 8'h3c, 8'h83, 8'h53, 8'h99, 8'h61,

    8'h17, 8'h2b, 8'h04, 8'h7e, 8'hba, 8'h77, 8'hd6, 8'h26,
    8'he1, 8'h69, 8'h14, 8'h63, 8'h55, 8'h21, 8'h0c, 8'h7d
  };

  // Drive output
  assign data_o = (op_i == CIPH_FWD) ? SBOX_FWD[data_i] :
                  (op_i == CIPH_INV) ? SBOX_INV[data_i] : SBOX_FWD[data_i];

endmodule
