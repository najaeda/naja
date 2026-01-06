/*
  Error: range connection to scalar in concatenation
*/

module model(input[1:0] i);
endmodule

module test();
  wire n0, n1;
  model inst(.i({n0, n1[0]}));
endmodule