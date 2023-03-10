/*
  Simple verilog file testing basic structures
*/

module mod0(input i0, output o0);
  //Might be a blackbox in the future...
endmodule

module mod1(input[31:0] i, output[31:0] o);
  //Might be a blackbox in the future...
endmodule

module test(input i, output o, inout io);
  wire net0;
  wire net1, net2, net3;
  wire [31:0] net4;
  supply0 constant0;
  supply1 constant1;

  mod0 inst0(.i0(net1), .o0(net4[31]));
  mod1 inst1(.i(net4), .o());
endmodule