/*
  Error: referencing bit slice on scalar net.
*/

module model(i, o);
  input i;
  output o;
  input error;
endmodule

module test();
  wire n0, n1, n2;
  model inst(.i(n0), .o(n1), .error(n2));
endmodule