/*
  test_gates1: Test other gates.
  Note: Functionality is dummy, just to test the parsing and construction of the netlist.
*/

module top(input A, B, C, D, E, output [3:0] OUT);
  nand (OUT[0], A, B, C);
  nor (OUT[1], A, B, C, D, E);
  xnor (OUT[2], A, B, C, D);
  nand (OUT[3], A, B, C);
endmodule