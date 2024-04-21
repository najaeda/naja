/*
  Simple verilog file testing basic constructs
*/

module mod0(input i0, output o0);
  //should be a blackbox
endmodule

module mod1(input[4:0] i, output[4:0] o);
  //should be a blackbox
endmodule

module test(input i, output o, inout io);
  wire net0;
  wire net1, net2, net3;
  wire [3:-1] net4;
  supply0 constant0;
  supply1 constant1;

  mod0 inst0(.i0(net1), .o0(net4[3]));
  mod0 inst1(.i0(1'b0), .o0(net4[3]));
  mod1 inst2(.i(5'h1A), .o(net4));
  mod1 inst3(.i({ net1, net4[1:0], 2'h2 }), .o());
endmodule