module seq_timing_list_unsupported(
  input logic clk,
  input logic rst,
  input logic d,
  output logic q
);
  // Unsupported by constructor: statement-list wrapper (multiple statements)
  // while extracting a timed sequential statement.
  always begin
    if (rst) q <= 1'b0;
    else q <= d;
    q <= d;
  end
endmodule
