module model(input [2:-2] i0, input [-2:2] i1, output [2:-2] o0, output [-2:2] o1);
endmodule //model

module top();
wire [2:-2] bus0;
wire [-2:2] bus1;
wire [2:-2] bus2;
wire [-2:2] bus3;

model instance1(.i0(), .i1(), .o0({bus0[2:0], bus2[-1:-2]}), .o1());

model instance2(.i0({bus0[2:0], bus2[-1:-2]}), .i1(), .o0(), .o1());
endmodule //top
