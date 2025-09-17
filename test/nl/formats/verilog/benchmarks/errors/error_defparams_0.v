/*
  Error: more than 3 members defparam
*/

module model(input [2:0] i);
endmodule

module test();
  wire n0, n1, n2;
  model inst(.i({n0, n1, n2}));
  defparam wrong.inst.INIT = "16hFFFF";
endmodule