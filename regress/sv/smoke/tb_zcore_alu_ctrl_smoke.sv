// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module tb_zcore_alu_ctrl_smoke;
  logic [6:0] alu_op;
  logic [2:0] alu_funct3;
  logic [6:0] alu_funct7;
  wire [4:0] alu_inst_type;
  int failures = 0;

  z_core_alu_ctrl dut (.*);

  task automatic check(
    input logic [6:0] operation,
    input logic [2:0] funct3,
    input logic [6:0] funct7,
    input logic [4:0] expected
  );
    alu_op = operation;
    alu_funct3 = funct3;
    alu_funct7 = funct7;
    #1;
    if (alu_inst_type !== expected) begin
      $display("ZCORE_ALU_CTRL_SMOKE_FAIL op=%b f3=%b f7=%b type=%0d/%0d",
               operation, funct3, funct7, alu_inst_type, expected);
      failures++;
    end
  endtask

  initial begin
    check(7'b0110011, 3'b000, 7'b0000000, 5'd0);  // ADD
    check(7'b0110011, 3'b000, 7'b0100000, 5'd1);  // SUB
    check(7'b0110011, 3'b001, 7'b0000000, 5'd2);  // SLL
    check(7'b0110011, 3'b010, 7'b0000000, 5'd3);  // SLT
    check(7'b0110011, 3'b011, 7'b0000000, 5'd4);  // SLTU
    check(7'b0110011, 3'b100, 7'b0000000, 5'd5);  // XOR
    check(7'b0110011, 3'b101, 7'b0000000, 5'd6);  // SRL
    check(7'b0110011, 3'b101, 7'b0100000, 5'd7);  // SRA
    check(7'b0110011, 3'b110, 7'b0000000, 5'd8);  // OR
    check(7'b0110011, 3'b111, 7'b0000000, 5'd9);  // AND
    check(7'b1100011, 3'b000, 7'b0000000, 5'd10); // BEQ
    check(7'b1100011, 3'b001, 7'b0000000, 5'd11); // BNE
    check(7'b1100011, 3'b100, 7'b0000000, 5'd12); // BLT
    check(7'b1100011, 3'b101, 7'b0000000, 5'd13); // BGE
    check(7'b1100011, 3'b110, 7'b0000000, 5'd14); // BLTU
    check(7'b1100011, 3'b111, 7'b0000000, 5'd15); // BGEU
    check(7'b0110011, 3'b000, 7'b0000001, 5'd16); // MUL
    check(7'b0110011, 3'b001, 7'b0000001, 5'd17); // MULH
    check(7'b0110011, 3'b010, 7'b0000001, 5'd18); // MULHSU
    check(7'b0110011, 3'b011, 7'b0000001, 5'd19); // MULHU
    check(7'b0110011, 3'b100, 7'b0000001, 5'd20); // DIV
    check(7'b0110011, 3'b101, 7'b0000001, 5'd21); // DIVU
    check(7'b0110011, 3'b110, 7'b0000001, 5'd22); // REM
    check(7'b0110011, 3'b111, 7'b0000001, 5'd23); // REMU

    if (failures != 0)
      $fatal(1, "Z-Core ALU control smoke had %0d failure(s)", failures);
    $display("ZCORE_ALU_CTRL_SMOKE_PASS");
    $finish;
  end
endmodule
