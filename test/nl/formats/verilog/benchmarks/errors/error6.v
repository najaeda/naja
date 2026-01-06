/*
  Error: unknown bus net in concatenation error 
*/

module model(input[1:0] i, output[1:0] o);
endmodule

module test();
  wire n0, n1, n2;
  model inst(.i({n1, n2}), .o({n3, error[0]})); // error: unknown bus net bit 'error[0]'
endmodule