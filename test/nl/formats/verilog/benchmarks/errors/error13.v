/*
  Error: connect concatenation to a scalar term error
*/

module model(input i);
endmodule

module test();
  wire n0, n1;
  model inst(.i({n0, n1}));
endmodule