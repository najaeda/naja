// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
//
// Sized bit-literal parameter canonicalization coverage.
// Sized, non-negative based literals on bit-typed parameters are stored in
// the canonical form "<width>['s]'b<msb...lsb>"; unsized and negated literals
// have no faithful literal-level canonical form and keep their source text.

module test_param_canonicalization();
wire a;
wire [17:0] n;
wire [7:0] a8;
wire [7:0] m;
wire [7:0] m2;

LUT4 #(.INIT(16'h5054))   i_hex     (.I0(a), .Q(n[0]));
LUT4 #(.INIT(16'h00_01))  i_sep     (.I0(a), .Q(n[1]));
LUT4 #(.INIT(16'o777))    i_oct     (.I0(a), .Q(n[2]));
LUT4 #(.INIT(16'd5))      i_dec     (.I0(a), .Q(n[3]));
LUT4 #(.INIT(16'sh000F))  i_sig     (.I0(a), .Q(n[4]));
LUT4 #(.INIT(16'h00x1))   i_x       (.I0(a), .Q(n[5]));
LUT4 #(.INIT(16'hz1))     i_zext    (.I0(a), .Q(n[6]));
LUT4 #(.INIT('hF))        i_unsized (.I0(a), .Q(n[7]));
LUT4                      i_neg     (.I0(a), .Q(n[8]));

// Binary base with separators.
LUT4 #(.INIT(16'b0101_0000_0101_0100)) i_bin (.I0(a), .Q(n[9]));
// Signed literal whose top bit is 1: the 's marker decides sign extension in
// wider contexts, so it must survive canonicalization.
LUT4 #(.INIT(4'sb1111))   i_stop    (.I0(a), .Q(n[10]));
// Signed literal with unknown digits.
LUT4 #(.INIT(4'sb1x01))   i_sx      (.I0(a), .Q(n[11]));
// Decimal base, lone x digit: every bit unknown.
LUT4 #(.INIT(16'dx))      i_decx    (.I0(a), .Q(n[12]));
// More digit bits than the declared size: extra leftmost bits are truncated.
LUT4 #(.INIT(4'h1F))      i_trunc   (.I0(a), .Q(n[13]));
// Leftmost digit x: x-extends to the declared width.
LUT4 #(.INIT(16'hx1))     i_xpad    (.I0(a), .Q(n[14]));

// Same three shapes through defparam instead of #(...).
LUT4                      i_sdef    (.I0(a), .Q(n[15]));
LUT4                      i_sxdef   (.I0(a), .Q(n[16]));
LUT4                      i_negx    (.I0(a), .Q(n[17]));

// Negated literals evaluate at the context width, which is unknown here, so
// the defparam overrides below keep their source form (with the sign that
// Number::getString() would otherwise drop).
defparam i_neg.INIT   = -16'h0001;
defparam i_negx.INIT  = -4'bx001;
// Signed values through defparam canonicalize exactly like #(...) overrides.
defparam i_sdef.INIT  = 4'sb1111;
defparam i_sxdef.INIT = 4'sb1x01;

// WIDTH is a decimal-typed parameter and keeps its source form; INIT is
// bit-typed and gets canonicalized; MODE is string-typed.
MEM8 #(.WIDTH(8), .INIT(8'hA5), .MODE("fast")) i_mem (.D(a8), .Q(m));
MEM8 i_mem2 (.D(a8), .Q(m2));
defparam i_mem2.WIDTH = 16;
defparam i_mem2.MODE = "slow";

endmodule
