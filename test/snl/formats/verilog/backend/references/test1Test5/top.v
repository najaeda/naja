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
wire n2;
wire n3;
wire n4;

model #(
  .PARAM0("1111"),
  .PARAM1("TRUE"),
  .PARAM2(4h'F),
  .PARAM3(152)
) instance1 (
  .i0(5'h00),
  .i1(),
  .o0({bus0[0:-1], DUMMY, DUMMY, net_0}),
  .o1()
);

model instance2 (
  .i0(5'h13),
  .i1({2'b01, n2, n3, n4}),
  .o0(),
  .o1()
);
endmodule //top
