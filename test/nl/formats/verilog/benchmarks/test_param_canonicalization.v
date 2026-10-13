// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
//
// Sized bit-literal parameter canonicalization coverage.
// Sized, non-negative based literals on bit-typed parameters are stored in
// the canonical form "<width>['s]'b<msb...lsb>"; unsized and negated literals
// have no faithful literal-level canonical form and keep their source text.

module test_param_canonicalization();
wire a;
wire [8:0] n;
wire [7:0] a8;
wire [7:0] m;

LUT4 #(.INIT(16'h5054))   i_hex     (.I0(a), .Q(n[0]));
LUT4 #(.INIT(16'h00_01))  i_sep     (.I0(a), .Q(n[1]));
LUT4 #(.INIT(16'o777))    i_oct     (.I0(a), .Q(n[2]));
LUT4 #(.INIT(16'd5))      i_dec     (.I0(a), .Q(n[3]));
LUT4 #(.INIT(16'sh000F))  i_sig     (.I0(a), .Q(n[4]));
LUT4 #(.INIT(16'h00x1))   i_x       (.I0(a), .Q(n[5]));
LUT4 #(.INIT(16'hz1))     i_zext    (.I0(a), .Q(n[6]));
LUT4 #(.INIT('hF))        i_unsized (.I0(a), .Q(n[7]));
LUT4                      i_neg     (.I0(a), .Q(n[8]));

// A negated literal evaluates at the context width, which is unknown here,
// so the defparam override below keeps its source form "-16'h0001".
defparam i_neg.INIT = -16'h0001;

// WIDTH is a decimal-typed parameter and must keep its source form; INIT is
// bit-typed and gets canonicalized.
MEM8 #(.WIDTH(8'hF), .INIT(8'hA5)) i_mem (.D(a8), .Q(m));

endmodule
