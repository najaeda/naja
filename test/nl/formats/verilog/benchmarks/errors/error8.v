/*
  Error: missing net on right side of assign
*/

module test();
  wire n0;

  assign n0 = n1;
endmodule