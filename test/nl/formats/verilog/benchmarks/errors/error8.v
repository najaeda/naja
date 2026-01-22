/*
  Error: missing bus net on right side of assign
*/

module test();
  wire n0;

  assign n0 = n1[0]; // error: unknown bus net 'n1'
endmodule