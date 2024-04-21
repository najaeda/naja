module addaccu(input[3:0] a, input[3:0] b, input sel, input ck, output[3:0] s);

wire [3:0] accu;
wire [3:0] x;
wire [3:0] sum;
wire [3:0] r;

always @(*) begin
    x = (sel == 1'b0) ? a : accu;
end

assign sum = b ^ x ^ r;
assign r[3:1] = (b[2:0] & x[2:0]) | (b[2:0] & r[2:0]) | (x[2:0] & r[2:0]);
assign r[0] = 1'b0;

assign s = sum;

always @(posedge ck) begin
  accu <= sum;
end

endmodule
