// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
//
// Dump/reload round trip for canonicalized parameter values: every override
// below must survive SNLVRLDumper output followed by SNLVRLConstructor
// re-parsing unchanged. Negated literals are covered separately in
// test_param_canonicalization.v: they keep source form, and the #( ... )
// subset of the Verilog expression grammar has no unary minus, so a dumped
// negative value could not be parsed back here.

module test_param_canonicalization_rt();
wire a;
wire [3:0] n;

LUT4 #(.INIT(16'h5054))  i_hex  (.I0(a), .Q(n[0]));
LUT4 #(.INIT(4'sb1111))  i_stop (.I0(a), .Q(n[1]));
LUT4 #(.INIT(4'sb1x01))  i_sx   (.I0(a), .Q(n[2]));
LUT4 #(.INIT('hF))       i_unsized (.I0(a), .Q(n[3]));

endmodule
