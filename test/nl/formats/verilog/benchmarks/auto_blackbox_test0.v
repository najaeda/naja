/*
* This is a test file for the auto blackbox generation.
* It contains various test cases to ensure the functionality
* of the blackbox generation process.
*/

module test();
  wire[3:0] net;
  auto_blackbox0 ins0(
    .A(1'b0),
    .B(1'b1),
    .C({1'b0, 1'b1, 1'b0}),
    .OUT({net[0], net[1]}),
    .COUT(net[2])
  );
endmodule