module model(input [2:-2] i0, input [-2:2] i1, output [2:-2] o0, output [-2:2] o1);
parameter PARAM0 = "0000" ;
parameter PARAM1 = "FALSE" ;
parameter PARAM2 = 4h'0 ;
parameter PARAM3 = 10 ;

endmodule //model

module top();
wire [2:-2] bus0;
wire [-2:2] bus1;
wire [2:-2] bus2;
wire [-2:2] bus3;
wire net_0;
wire net_2;

model instance1 (
  .i0(),
  .i1(),
  .o0({net_0, bus0[-2], net_2, bus0[-1:0]}),
  .o1()
);

model instance2 (
  .i0(),
  .i1(),
  .o0(),
  .o1()
);
endmodule //top
