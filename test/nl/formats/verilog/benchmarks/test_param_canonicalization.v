// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
//
// Sized bit-literal parameter canonicalization coverage.
// Every INIT override below must be stored as "<width>'b<msb...lsb>".

module test_param_canonicalization();
wire a;
wire [6:0] n;
wire [7:0] a8;
wire [7:0] m;

LUT4 #(.INIT(16'h5054))   i_hex     (.I0(a), .Q(n[0]));
LUT4 #(.INIT(16'h00_01))  i_sep     (.I0(a), .Q(n[1]));
LUT4 #(.INIT(16'o777))    i_oct     (.I0(a), .Q(n[2]));
LUT4 #(.INIT(16'd5))      i_dec     (.I0(a), .Q(n[3]));
LUT4 #(.INIT(16'sh000F))  i_sig     (.I0(a), .Q(n[4]));
LUT4 #(.INIT(16'h00x1))   i_x       (.I0(a), .Q(n[5]));
LUT4 #(.INIT('hF))        i_unsized (.I0(a), .Q(n[6]));

// WIDTH is a decimal-typed parameter and must keep its source form; INIT is
// bit-typed and gets canonicalized.
MEM8 #(.WIDTH(8'hF), .INIT(8'hA5)) i_mem (.D(a8), .Q(m));

endmodule
