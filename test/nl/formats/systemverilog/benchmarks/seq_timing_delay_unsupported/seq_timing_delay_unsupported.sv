module seq_timing_delay_unsupported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported timing extraction case: delay control is not a supported clock event.
  always begin
    #1 if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
