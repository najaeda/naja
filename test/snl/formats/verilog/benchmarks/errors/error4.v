/*
  Error: module port collision
*/

module model(input i, output o);
endmodule

module test();
  wire n0, n1, n2;
  model inst(.i(n0), .error(n1), .o(n2));
endmodule