/*
  Error: unknown net in concatenation error 
*/

module model(input[1:0] i);
endmodule

module test();
  wire n0, n1, n2;
  model inst(.i({n0, n1, n2}));
endmodule