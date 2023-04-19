/*
  Testing loading with initial primitives loading
  Testing assigns
*/

module model0(i, o, io);
  input i;
  output o;
  inout io;

  LUT4 #(.INIT(16'h1000)) lut(.I0(i), .Q(o));
endmodule

//same kind of model but with ports also declared as wires
module model1(i, o, io);
  input i;
  wire i;
  output o;
  wire o;
  inout io;
  wire io;

  LUT4 #(.INIT(16'h1000)) lut(.I0(i), .Q(o));
endmodule

module test(input i, output o, inout io);
  wire n0;
  wire n1;
  wire n2;
  wire n3;

  model0 inst0(.i(n0), .o(n1), .io(n2));
  model1 inst1(.i(n2), .o(i), .io(io));
  assign n0 = n3;
  assign n3 = 1'b0; 
endmodule