module top (clk
);
 input clk;

 wire _015_;
 wire _031_;
 wire clknet_1_0__leaf_clk;
 wire \dpath.a_lt_b$in0[12] ;
 wire \dpath.a_lt_b$in1[12] ;
 wire net44;
 wire net45;

 cell_def _tray_size2_66 (.CK(clknet_1_0__leaf_clk),
    .D0(_031_),
    .D1(_015_),
    .QN0(\dpath.a_lt_b$in1[12] ),
    .QN1(\dpath.a_lt_b$in0[12] ),
    .SE(net44),
    .SI(net45));
endmodule
