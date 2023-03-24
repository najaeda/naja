/*
  Testing loading with initial primitives loading 
*/

module model(i, o, io);
  input i;
  output o;
  inout io;

  LUT4 lut(.I0(i), .Q(o));
endmodule

module test(input i, output o, inout io);
  wire net0;

  model inst();
endmodule