/*
  Error: instance parameter referencing parameter not existing in model 
*/

module model();
endmodule

module test();
  model #(.INIT(16'h1000)) instance();
endmodule