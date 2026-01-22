/*
  Error: unsupported type in instance connection
*/

module model(input i);
endmodule

module test();
  model inst(.i("FOO"));
endmodule