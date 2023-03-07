module model();
  wire net0;

  LUT4 lut(.I0(net0));
endmodule

module test(input i, output o, inout io);
  wire net0;

  model inst();
endmodule