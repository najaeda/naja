// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module tb_zcore_top_smoke;
  logic clk = 1'b0;
  logic rstn = 1'b0;
  logic uart_rx = 1'b1;
  logic timer_ext_event_i = 1'b0;
  wire uart_tx;
  tri [63:0] gpio_pins;

  z_core_top dut (
    .clk(clk),
    .rstn(rstn),
    .uart_rx(uart_rx),
    .uart_tx(uart_tx),
    .gpio_pins(gpio_pins),
    .timer_ext_event_i(timer_ext_event_i)
  );

  always #5 clk = ~clk;

  initial begin
    repeat (10) @(posedge clk);
    rstn = 1'b1;
    repeat (200) @(posedge clk);

    if ($isunknown(uart_tx))
      $fatal(1, "Z-Core UART output became unknown");
    if ($isunknown(gpio_pins))
      $fatal(1, "Z-Core GPIO output became unknown");

    $display("ZCORE_TOP_SMOKE_PASS");
    $finish;
  end
endmodule
