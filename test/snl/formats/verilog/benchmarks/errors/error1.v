/*
  Error: referencing bit slice on scalar net.
*/

module model(input[2:0] i);
endmodule

module test();
  wire net0;

  model inst(.i(net0[2:0]));
endmodule