/*
  Error: missing net on right side of assign
*/

module test();
  wire n0;

  assign n1 = n0;
endmodule