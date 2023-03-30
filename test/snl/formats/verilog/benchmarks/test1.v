/*
  Testing loading with initial primitives loading
  Testing assigns
*/

module model(i, o, io);
  input i;
  output o;
  inout io;

  LUT4 #(.INIT(16'h1000)) lut(.I0(i), .Q(o));
endmodule

module test(input i, output o, inout io);
  wire n0;
  wire n1;
  wire n2;
  wire n3;

  model inst(.i(n0), .o(n1), .io(n2));
  assign n0 = n3;
  assign n3 = 1'b0; 
endmodule