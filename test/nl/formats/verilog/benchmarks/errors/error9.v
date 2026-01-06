/*
  Error: missing bus net on right side of assign
*/

module test();
  wire n0;

  assign n1 = n0[3];
endmodule