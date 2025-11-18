/*
* This is a test file for the auto blackbox generation.
* It contains various test cases to ensure the functionality
* of the blackbox generation process.
*/

module test1();
  wire[3:0] net0, net1;
  
  auto_blackbox0 ins0(
    .A(1'b0),
    .B(1'b1),
    .C({1'b0, 1'b1, 1'b0}),
    .OUT({net0[0], net0[1]}),
    .COUT(net0[2])
  );

  auto_blackbox0 ins1(
    .A(1'b0),
    .B(1'b1),
    .C({1'b0, 1'b1, 1'b0}),
    .OUT({net1[0], net1[1]}),
    .COUT(net1[2])
  );

endmodule