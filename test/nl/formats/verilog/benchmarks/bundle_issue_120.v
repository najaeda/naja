module top (clk
);
 input clk;

 wire data_in_0;
 wire data_in_1;
 wire clock_net;
 wire \data_out[0] ;
 wire \data_out[1] ;
 wire scan_enable;
 wire scan_input;

 cell_def u_cell (.CK(clock_net),
    .D0(data_in_1),
    .D1(data_in_0),
    .QN0(\data_out[1] ),
    .QN1(\data_out[0] ),
    .SE(scan_enable),
    .SI(scan_input));
endmodule
