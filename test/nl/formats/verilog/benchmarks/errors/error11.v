/*
  Error: simple port declaration has no corresponding declaration in 
  module implementation
*/

module model(a, b, c);
input a;
//missing b
output c;
endmodule

module test();
  model instance();
endmodule