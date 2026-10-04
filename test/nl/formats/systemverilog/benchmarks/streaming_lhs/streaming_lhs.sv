// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module streaming_lhs_top(
  input logic [19:0] value,
  input logic enable,
  output logic [9:0] forward_o, reverse_o, nested_o, selected_o,
  output logic [7:0] swap_o,
  output logic [15:0] array_o,
  output logic [15:0] multi_o, stream_rhs_o
);
  logic [3:0] a, b;
  logic [3:0] upper;
  logic [11:0] lower;
  logic [7:0] bytes [0:1];
  logic [15:0] temp;
  always_comb begin
    forward_o = '0;
    if (enable) {>>{forward_o}} = value;
    {<<4{reverse_o}} = value[9:0];
    {<<4{{<<2{nested_o}}}} = value[9:0];
    selected_o = 10'h301;
    {<<2{selected_o[7:2]}} = value[5:0];
    a = value[3:0];
    b = value[7:4];
    {>>{a,b}} = {b,a};
    swap_o = {a,b};
    {>>{bytes}} = {2{value[7:0]}};
    array_o = {bytes[0],bytes[1]};
    {>>{upper,lower}} = value[15:0];
    multi_o = {lower,upper};
    temp = value[15:0];
    temp ^= 16'h1234;
    {>>{stream_rhs_o}} = {<<8{temp}};
  end
endmodule
