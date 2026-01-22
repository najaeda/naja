/*
  Testing loading with initial primitives loading
  Testing assigns
*/

module model0(io, o, i);
  //order of ports in memory should be: io, o, i
  input i;
  output o;
  inout io;

  LUT4 #(.INIT(16'h1000)) lut(.I0(i), .Q(o));
endmodule

//same kind of model but with ports also declared as wires
//and different ports orders
module model1(i, o, io);
  //order of ports in memory should be: i, o, io
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
  wire [3:0] n4;
  wire [1:1] n5;
  wire n6;

  model0 inst0(.i(n0), .o(n1), .io(n2));
  model1 inst1(.i(n2), .o(i), .io(io));
  model1 inst2(n2, i, io);
  assign n0 = n3;
  assign n3 = 1'b0;
  assign { n4[3:2], n4[1:0] } = { n0, n5[1], 2'h2 };
  assign n6 = n5[1]; 
endmodule