/*
  Testing empty connection
*/

module model0(input A);
endmodule

module test_empty_instance_connection();
  model0 inst0(.A());
endmodule