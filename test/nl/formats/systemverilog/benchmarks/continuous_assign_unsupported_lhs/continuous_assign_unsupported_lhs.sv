module continuous_assign_unsupported_lhs(
  input logic a,
  output string y
);
  // Unsupported LHS type; continuous-assign lowering skips unresolved lhs net.
  assign y = a;
endmodule
