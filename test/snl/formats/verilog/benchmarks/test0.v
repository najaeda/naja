module mod0(input i0, output o0);
endmodule

module test(input i, output o, inout io);
  wire net0;
  wire net1, net2, net3;
  wire net4[31:0];
  supply0 constant0;
  supply1 constant1;

  mod0 inst0(.i0(), .o0());
endmodule