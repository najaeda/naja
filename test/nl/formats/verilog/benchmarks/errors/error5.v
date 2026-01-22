/*
  Error: unknown bus net bit error 
*/

module model(input i, output o);
endmodule

module test();
  wire n0, n1;
  model inst(.i(n0), .o(error[0])); // error: unknown bus net bit 'error[0]'
endmodule