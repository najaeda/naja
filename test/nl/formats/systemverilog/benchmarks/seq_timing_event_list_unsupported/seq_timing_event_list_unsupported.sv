module seq_timing_event_list_unsupported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported timing extraction case: event list contains multiple events.
  always_ff @(posedge clk or posedge rst) begin
    if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
