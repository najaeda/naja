/*
  Error: unknown net error 
*/

module model(input i, output o);
endmodule

module test();
  wire n0, n1;
  model inst(.i(n0), .o(error));
endmodule