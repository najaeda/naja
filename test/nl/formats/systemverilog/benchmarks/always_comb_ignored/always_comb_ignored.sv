module always_comb_ignored(
  input logic a,
  input logic b,
  output logic y
);
  always_comb begin
    y = a & b;
  end
endmodule
