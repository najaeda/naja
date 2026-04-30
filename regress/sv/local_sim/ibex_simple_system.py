#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Run Ibex simple-system hello_test with a Naja-generated ibex_top."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys


REPO_ROOT = Path(__file__).resolve().parents[3]


def run(args: list[str], *, cwd: Path | None = None) -> None:
    print("$ " + (" ".join(args) if cwd is None else f"(cd {cwd} && {' '.join(args)})"), flush=True)
    subprocess.run(args, cwd=str(cwd) if cwd else None, check=True)


def find_riscv_tool_prefix() -> str:
    for prefix in ("riscv32-unknown-elf-", "riscv64-unknown-elf-"):
        if shutil.which(prefix + "gcc") and shutil.which(prefix + "objcopy"):
            return prefix
    raise SystemExit(
        "missing RISC-V toolchain: expected riscv32-unknown-elf-{gcc,objcopy} "
        "or riscv64-unknown-elf-{gcc,objcopy}"
    )


def write_word_vmem(binary_path: Path, vmem_path: Path, base_address: int) -> None:
    data = binary_path.read_bytes()
    with vmem_path.open("w", encoding="utf-8") as stream:
        stream.write(f"@{base_address // 4:08x}\n")
        for offset in range(0, len(data), 4):
            word_bytes = data[offset:offset + 4].ljust(4, b"\x00")
            stream.write(f"{int.from_bytes(word_bytes, 'little'):08x}\n")


def patch_ibex_common_csr_asm(repo_dir: Path) -> None:
    common_dir = repo_dir / "examples" / "sw" / "simple_system" / "common"
    common_c_path = common_dir / "simple_system_common.c"
    common_h_path = common_dir / "simple_system_common.h"

    def csrw_x0_word(csr: int) -> int:
        return (csr << 20) | 0x1073

    pcount_reset_words = [
        csrw_x0_word(0xB02),  # minstret
        csrw_x0_word(0xB00),  # mcycle
        *[csrw_x0_word(0xB00 + counter) for counter in range(3, 32)],
        csrw_x0_word(0xB82),  # minstreth
        csrw_x0_word(0xB80),  # mcycleh
        *[csrw_x0_word(0xB80 + counter) for counter in range(3, 32)],
    ]
    pcount_reset_asm = (
        "  asm volatile(\n"
        + "\n".join(f'      ".word 0x{word:08x}\\n"' for word in pcount_reset_words)
        + ");"
    )

    def apply_replacements(path: Path, replacements: dict[str, str]) -> None:
        text = path.read_text(encoding="utf-8")
        for old, new in replacements.items():
            if old in text:
                text = text.replace(old, new, 1)
        path.write_text(text, encoding="utf-8")

    common_c_text = common_c_path.read_text(encoding="utf-8")
    common_c_text = re.sub(
        r"void pcount_reset\(\) \{\n  asm volatile\(\n.*?\);\n\}",
        lambda _: "void pcount_reset() {\n" + pcount_reset_asm + "\n}",
        common_c_text,
        count=1,
        flags=re.DOTALL,
    )
    common_c_path.write_text(common_c_text, encoding="utf-8")

    # Some GitHub runner RISC-V binutils are too old to understand either
    # -march=rv32imc_zicsr or .option arch,+zicsr. Use raw CSR encodings in
    # the patched simple-system firmware instead.
    apply_replacements(common_c_path, {
        '__asm__ volatile("csrr %0, mepc;" : "=r"(result));': (
            '__asm__ volatile(".word 0x341027f3\\n"\n'
            '                   "mv %0, a5\\n" : "=r"(result) :: "a5");'
        ),
        '__asm__ volatile("csrr %0, mcause;" : "=r"(result));': (
            '__asm__ volatile(".word 0x342027f3\\n"\n'
            '                   "mv %0, a5\\n" : "=r"(result) :: "a5");'
        ),
        '__asm__ volatile("csrr %0, mtval;" : "=r"(result));': (
            '__asm__ volatile(".word 0x343027f3\\n"\n'
            '                   "mv %0, a5\\n" : "=r"(result) :: "a5");'
        ),
        'asm volatile("csrs  mie, %0\\n" : : "r"(0x80));': (
            'asm volatile("li a5, 0x80\\n"\n'
            '             ".word 0x3047a073\\n" ::: "a5");'
        ),
        'asm volatile("csrs  mstatus, %0\\n" : : "r"(0x8));': (
            'asm volatile("li a5, 0x8\\n"\n'
            '             ".word 0x3007a073\\n" ::: "a5");'
        ),
        'void timer_disable(void) { asm volatile("csrc  mie, %0\\n" : : "r"(0x80)); }': (
            'void timer_disable(void) {\n'
            '  asm volatile("li a5, 0x80\\n"\n'
            '               ".word 0x3047b073\\n" ::: "a5");\n'
            '}'
        ),
    })
    common_c_text = common_c_path.read_text(encoding="utf-8")
    common_c_text = re.sub(
        r'__asm__ volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrr %0, mepc;\\n"\n'
        r'\s*"\.option pop\\n" : "=r"\(result\)\);',
        lambda _: (
            '__asm__ volatile(".word 0x341027f3\\n"\n'
            '                   "mv %0, a5\\n" : "=r"(result) :: "a5");'
        ),
        common_c_text,
        count=1,
    )
    common_c_text = re.sub(
        r'__asm__ volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrr %0, mcause;\\n"\n'
        r'\s*"\.option pop\\n" : "=r"\(result\)\);',
        lambda _: (
            '__asm__ volatile(".word 0x342027f3\\n"\n'
            '                   "mv %0, a5\\n" : "=r"(result) :: "a5");'
        ),
        common_c_text,
        count=1,
    )
    common_c_text = re.sub(
        r'__asm__ volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrr %0, mtval;\\n"\n'
        r'\s*"\.option pop\\n" : "=r"\(result\)\);',
        lambda _: (
            '__asm__ volatile(".word 0x343027f3\\n"\n'
            '                   "mv %0, a5\\n" : "=r"(result) :: "a5");'
        ),
        common_c_text,
        count=1,
    )
    common_c_text = re.sub(
        r'asm volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrs  mie, %0\\n"\n'
        r'\s*"\.option pop\\n" : : "r"\(0x80\)\);',
        lambda _: (
            'asm volatile("li a5, 0x80\\n"\n'
            '             ".word 0x3047a073\\n" ::: "a5");'
        ),
        common_c_text,
        count=1,
    )
    common_c_text = re.sub(
        r'asm volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrs  mstatus, %0\\n"\n'
        r'\s*"\.option pop\\n" : : "r"\(0x8\)\);',
        lambda _: (
            'asm volatile("li a5, 0x8\\n"\n'
            '             ".word 0x3007a073\\n" ::: "a5");'
        ),
        common_c_text,
        count=1,
    )
    common_c_text = re.sub(
        r'asm volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrc  mie, %0\\n"\n'
        r'\s*"\.option pop\\n" : : "r"\(0x80\)\);',
        lambda _: (
            'asm volatile("li a5, 0x80\\n"\n'
            '               ".word 0x3047b073\\n" ::: "a5");'
        ),
        common_c_text,
        count=1,
    )
    common_c_path.write_text(common_c_text, encoding="utf-8")

    apply_replacements(common_h_path, {
        '#define PCOUNT_READ(name, dst) asm volatile("csrr %0, " #name ";" : "=r"(dst))': (
            '#define PCOUNT_READ(name, dst) do { (dst) = 0; } while (0)'
        ),
        'asm volatile("csrw  0x320, %0\\n" : : "r"(inhibit_val));': (
            'asm volatile("mv a5, %0\\n"\n'
            '               ".word 0x32079073\\n" : : "r"(inhibit_val) : "a5");'
        ),
        'asm volatile("csrs 0x7c0, 1");': (
            'asm volatile(".word 0x7c00e073\\n");'
        ),
        'asm volatile("csrc 0x7c0, 1");': (
            'asm volatile(".word 0x7c00f073\\n");'
        ),
    })
    common_h_text = common_h_path.read_text(encoding="utf-8")
    common_h_text = re.sub(
        r'#define PCOUNT_READ\(name, dst\) \\\n'
        r'\s*asm volatile\("\.option push\\n" \\\n'
        r'\s*"\.option arch, \+zicsr\\n" \\\n'
        r'\s*"csrr %0, " #name ";\\n" \\\n'
        r'\s*"\.option pop\\n" : "=r"\(dst\)\)',
        lambda _: '#define PCOUNT_READ(name, dst) do { (dst) = 0; } while (0)',
        common_h_text,
        count=1,
    )
    common_h_text = re.sub(
        r'asm volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrw  0x320, %0\\n"\n'
        r'\s*"\.option pop\\n" : : "r"\(inhibit_val\)\);',
        lambda _: (
            'asm volatile("mv a5, %0\\n"\n'
            '               ".word 0x32079073\\n" : : "r"(inhibit_val) : "a5");'
        ),
        common_h_text,
        count=1,
    )
    common_h_text = re.sub(
        r'asm volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrs 0x7c0, 1\\n"\n'
        r'\s*"\.option pop\\n"\);',
        lambda _: 'asm volatile(".word 0x7c00e073\\n");',
        common_h_text,
        count=1,
    )
    common_h_text = re.sub(
        r'asm volatile\("\.option push\\n"\n'
        r'\s*"\.option arch, \+zicsr\\n"\n'
        r'\s*"csrc 0x7c0, 1\\n"\n'
        r'\s*"\.option pop\\n"\);',
        lambda _: 'asm volatile(".word 0x7c00f073\\n");',
        common_h_text,
        count=1,
    )
    common_h_path.write_text(common_h_text, encoding="utf-8")


def normalize_vc(vc_path: Path, generated_path: Path, primitives_path: Path) -> list[str]:
    base_dir = vc_path.parent
    args: list[str] = []
    for raw_line in vc_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line in {"--exe", "--top-module", "ibex_top"}:
            continue
        if line.startswith("-CFLAGS") or line.endswith(".vlt"):
            continue
        if line.startswith("+incdir+"):
            include_path = Path(line[len("+incdir+"):])
            args.append("+incdir+" + str(include_path if include_path.is_absolute() else base_dir / include_path))
            continue
        if line.startswith("-D") or line.startswith("+define+"):
            args.append(line)
            continue
        if line.endswith((".sv", ".v")):
            rtl_path = Path(line)
            rtl_path = rtl_path if rtl_path.is_absolute() else base_dir / rtl_path
            # The Naja dump contains the elaborated module hierarchy. Keep the
            # upstream packages needed by the simple-system wrapper, but avoid
            # compiling original RTL modules that would duplicate generated modules.
            if rtl_path.name.endswith("_pkg.sv"):
                args.append(str(rtl_path))

    args.extend([str(generated_path), str(primitives_path)])
    return args


def check_generated_netlist(generated_path: Path) -> None:
    constructor_source = (
        REPO_ROOT / "src" / "nl" / "formats" / "systemverilog" / "frontend" /
        "SNLSVConstructor.cpp"
    )
    if constructor_source.exists() and generated_path.stat().st_mtime < constructor_source.stat().st_mtime:
        raise SystemExit(
            f"generated netlist is older than {constructor_source}; regenerate with "
            "regress/sv/sv_regress.py run before launching local_sim directly: "
            f"{generated_path}"
        )

    in_regfile = False
    saw_waddr_bit0_one = False
    saw_waddr_bit1_one = False
    with generated_path.open("r", encoding="utf-8", errors="replace") as stream:
        for line in stream:
            if not in_regfile:
                if line.startswith("module ibex_register_file_ff"):
                    in_regfile = True
                continue
            if line.startswith("endmodule"):
                break
            if "waddr_a_i[0], 1'b1" in line:
                saw_waddr_bit0_one = True
            if "waddr_a_i[1], 1'b1" in line:
                saw_waddr_bit1_one = True
            if saw_waddr_bit0_one and saw_waddr_bit1_one:
                return

    if not (saw_waddr_bit0_one and saw_waddr_bit1_one):
        raise SystemExit(
            "generated ibex_register_file_ff write decoder still looks stale: "
            "all decoded write-address comparisons appear to target zero. "
            "Regenerate ibex_naja.v with the rebuilt Naja Python module before rerunning local_sim."
        )


def strip_instance_parameter_block(text: str, module_name: str, instance_name: str) -> str:
    pattern = re.compile(rf"{module_name}\s*#\s*\(", re.MULTILINE)
    match = pattern.search(text)
    if not match:
        return text

    depth = 1
    index = match.end()
    while index < len(text) and depth:
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
        index += 1
    while index < len(text) and text[index].isspace():
        index += 1
    if not text.startswith(instance_name, index):
        raise SystemExit(f"could not locate {module_name} instance {instance_name} after parameter block")
    return text[:match.start()] + f"{module_name} " + text[index:]


def patched_simple_system(repo_dir: Path, work_dir: Path) -> Path:
    source_path = repo_dir / "examples" / "simple_system" / "rtl" / "ibex_simple_system.sv"
    text = source_path.read_text(encoding="utf-8")
    text = text.replace("ibex_top_tracing #(", "ibex_top #(")
    text = strip_instance_parameter_block(text, "ibex_top", "u_top")
    text = text.replace(
        ".test_en_i                 (1'b0),",
        ".test_en_i                 (1'b1),",
    )
    text = text.replace(
        "  logic instr_err;\n\n"
        "  assign instr_gnt = instr_req;\n",
        "  logic instr_err;\n\n"
        "  logic rvfi_valid;\n"
        "  logic [31:0] rvfi_insn;\n"
        "  logic [4:0] rvfi_rd_addr;\n"
        "  logic [31:0] rvfi_rd_wdata;\n"
        "  logic [4:0] rvfi_rs1_addr;\n"
        "  logic [31:0] rvfi_rs1_rdata;\n"
        "  logic [4:0] rvfi_rs2_addr;\n"
        "  logic [31:0] rvfi_rs2_rdata;\n"
        "  logic [31:0] rvfi_pc_rdata;\n"
        "  logic [31:0] rvfi_pc_wdata;\n\n"
        "  assign instr_gnt = instr_req;\n",
    )
    text = text.replace(
        "      .double_fault_seen_o       (),\n\n"
        "      .fetch_enable_i            (ibex_pkg::IbexMuBiOn),",
        "      .double_fault_seen_o       (),\n\n"
        "      .rvfi_valid                (rvfi_valid),\n"
        "      .rvfi_insn                 (rvfi_insn),\n"
        "      .rvfi_rd_addr              (rvfi_rd_addr),\n"
        "      .rvfi_rd_wdata             (rvfi_rd_wdata),\n"
        "      .rvfi_rs1_addr             (rvfi_rs1_addr),\n"
        "      .rvfi_rs1_rdata            (rvfi_rs1_rdata),\n"
        "      .rvfi_rs2_addr             (rvfi_rs2_addr),\n"
        "      .rvfi_rs2_rdata            (rvfi_rs2_rdata),\n"
        "      .rvfi_pc_rdata             (rvfi_pc_rdata),\n"
        "      .rvfi_pc_wdata             (rvfi_pc_wdata),\n\n"
        "      .fetch_enable_i            (ibex_pkg::IbexMuBiOn),",
    )
    text = text.replace(
        '  export "DPI-C" function mhpmcounter_num;\n',
        """  bit naja_debug;
  int unsigned naja_cycle_count;
  int unsigned naja_instr_req_count;
  int unsigned naja_rvfi_count;
  int unsigned naja_simctrl_write_count;
  int unsigned naja_data_req_count;
  logic [31:0] naja_last_rvfi_pc;
  logic [31:0] naja_last_rvfi_next_pc;
  logic [31:0] naja_last_rvfi_insn;
  logic [4:0]  naja_last_rvfi_rd_addr;
  logic [31:0] naja_last_rvfi_rd_wdata;
  logic [31:0] naja_last_data_addr;
  logic [31:0] naja_last_data_wdata;
  logic        naja_last_data_we;
  bit naja_reset_jal_seen;

  initial begin
    naja_debug = 1'b0;
    void'($value$plusargs("naja_debug=%0b", naja_debug));
  end

  always_ff @(posedge clk_sys or negedge rst_sys_n) begin
    if (!rst_sys_n) begin
      naja_cycle_count <= 0;
      naja_instr_req_count <= 0;
      naja_rvfi_count <= 0;
      naja_simctrl_write_count <= 0;
      naja_data_req_count <= 0;
      naja_last_rvfi_pc <= 32'b0;
      naja_last_rvfi_next_pc <= 32'b0;
      naja_last_rvfi_insn <= 32'b0;
      naja_last_rvfi_rd_addr <= 5'b0;
      naja_last_rvfi_rd_wdata <= 32'b0;
      naja_last_data_addr <= 32'b0;
      naja_last_data_wdata <= 32'b0;
      naja_last_data_we <= 1'b0;
      naja_reset_jal_seen <= 1'b0;
    end else begin
      naja_cycle_count <= naja_cycle_count + 1;
      if (instr_req && instr_gnt) begin
        naja_instr_req_count <= naja_instr_req_count + 1;
      end
      if (rvfi_valid) begin
        naja_rvfi_count <= naja_rvfi_count + 1;
        naja_last_rvfi_pc <= rvfi_pc_rdata;
        naja_last_rvfi_next_pc <= rvfi_pc_wdata;
        naja_last_rvfi_insn <= rvfi_insn;
        naja_last_rvfi_rd_addr <= rvfi_rd_addr;
        naja_last_rvfi_rd_wdata <= rvfi_rd_wdata;
      end
      if (host_req[0] && host_gnt[0]) begin
        naja_data_req_count <= naja_data_req_count + 1;
        naja_last_data_addr <= host_addr[0];
        naja_last_data_wdata <= host_wdata[0];
        naja_last_data_we <= host_we[0];
        if (naja_debug) begin
          $display("NAJA_IBEX_DATA cycle=%0d addr=%08x we=%0b be=%04b wdata=%08x",
                   naja_cycle_count, host_addr[0], host_we[0], host_be[0], host_wdata[0]);
        end
      end
      if (device_req[SimCtrl] && device_we[SimCtrl]) begin
        naja_simctrl_write_count <= naja_simctrl_write_count + 1;
      end
      if (naja_debug && (naja_cycle_count < 20 || instr_req || rvfi_valid)) begin
        $display("NAJA_IBEX_DBG cycle=%0d rst=%0b instr_req=%0b instr_gnt=%0b instr_rvalid=%0b instr_addr=%08x instr_rdata=%08x rvfi_valid=%0b rvfi_pc=%08x rvfi_next_pc=%08x rvfi_insn=%08x rd=x%0d rd_wdata=%08x rs1=x%0d rs1_rdata=%08x rs2=x%0d rs2_rdata=%08x dec_jump_in=%0b dec_jump_set=%0b dec_branch_in=%0b dec_illegal=%0b",
                 naja_cycle_count, rst_sys_n, instr_req, instr_gnt, instr_rvalid,
                 instr_addr, instr_rdata, rvfi_valid, rvfi_pc_rdata, rvfi_pc_wdata, rvfi_insn,
                 rvfi_rd_addr, rvfi_rd_wdata, rvfi_rs1_addr, rvfi_rs1_rdata,
                 rvfi_rs2_addr, rvfi_rs2_rdata,
                 u_top.u_ibex_core.id_stage_i.decoder_i.jump_in_dec_o,
                 u_top.u_ibex_core.id_stage_i.decoder_i.jump_set_o,
                 u_top.u_ibex_core.id_stage_i.decoder_i.branch_in_dec_o,
                 u_top.u_ibex_core.id_stage_i.decoder_i.illegal_insn_o);
      end
      if (rvfi_valid && rvfi_pc_rdata == 32'h00100080 && rvfi_insn == 32'h2ce0006f) begin
        naja_reset_jal_seen <= 1'b1;
        if (naja_debug) begin
          $display("NAJA_IBEX_RESET_JAL pc=%08x next_pc=%08x expected_next_pc=0010034e dec_jump_in=%0b dec_jump_set=%0b dec_branch_in=%0b dec_illegal=%0b",
                   rvfi_pc_rdata, rvfi_pc_wdata,
                   u_top.u_ibex_core.id_stage_i.decoder_i.jump_in_dec_o,
                   u_top.u_ibex_core.id_stage_i.decoder_i.jump_set_o,
                   u_top.u_ibex_core.id_stage_i.decoder_i.branch_in_dec_o,
                   u_top.u_ibex_core.id_stage_i.decoder_i.illegal_insn_o);
        end
      end
    end
  end

  final begin
    int progress_fd;
    progress_fd = $fopen("naja_ibex_progress.log", "w");
    if (progress_fd != 0) begin
      $fdisplay(progress_fd, "cycles=%0d", naja_cycle_count);
      $fdisplay(progress_fd, "instr_reqs=%0d", naja_instr_req_count);
      $fdisplay(progress_fd, "rvfi_valid=%0d", naja_rvfi_count);
      $fdisplay(progress_fd, "simctrl_writes=%0d", naja_simctrl_write_count);
      $fdisplay(progress_fd, "data_reqs=%0d", naja_data_req_count);
      $fdisplay(progress_fd, "last_rvfi_pc=%08x", naja_last_rvfi_pc);
      $fdisplay(progress_fd, "last_rvfi_next_pc=%08x", naja_last_rvfi_next_pc);
      $fdisplay(progress_fd, "last_rvfi_insn=%08x", naja_last_rvfi_insn);
      $fdisplay(progress_fd, "last_rvfi_rd_addr=%0d", naja_last_rvfi_rd_addr);
      $fdisplay(progress_fd, "last_rvfi_rd_wdata=%08x", naja_last_rvfi_rd_wdata);
      $fdisplay(progress_fd, "last_data_addr=%08x", naja_last_data_addr);
      $fdisplay(progress_fd, "last_data_wdata=%08x", naja_last_data_wdata);
      $fdisplay(progress_fd, "last_data_we=%0d", naja_last_data_we);
      $fdisplay(progress_fd, "reset_jal_seen=%0d", naja_reset_jal_seen);
      $fclose(progress_fd);
    end
  end

""",
    )
    text = re.sub(
        r'\n(?:\s*export "DPI-C" function mhpmcounter_num;\n\n)?'
        r'\s*function automatic int unsigned mhpmcounter_num\(\);\n'
        r'.*?'
        r'\s*endfunction\n\n'
        r'(?:\s*export "DPI-C" function mhpmcounter_get;\n\n)?'
        r'\s*function automatic longint unsigned mhpmcounter_get\(int index\);\n'
        r'.*?'
        r'\s*endfunction\n',
        "\n",
        text,
        flags=re.DOTALL,
    )
    target_path = work_dir / "ibex_simple_system_naja.sv"
    target_path.write_text(text, encoding="utf-8")
    return target_path


def patched_simple_system_cpp(repo_dir: Path, work_dir: Path) -> Path:
    source_path = repo_dir / "examples" / "simple_system" / "ibex_simple_system.cc"
    text = source_path.read_text(encoding="utf-8")
    text = text.replace('#include "ibex_pcounts.h"\n', "")
    text = text.replace('#include "verilator_memutil.h"\n', "")
    if "#include <string>\n" not in text:
        text = text.replace("#include <iostream>\n", "#include <iostream>\n#include <string>\n")
    text = text.replace(
        'SimpleSystem::SimpleSystem(const char *ram_hier_path, int ram_size_words)\n'
        '    : _ram(ram_hier_path, ram_size_words, 4) {}\n',
        'SimpleSystem::SimpleSystem(const char *ram_hier_path, int ram_size_words) {}\n',
    )
    text = text.replace(
        '  _memutil.RegisterMemoryArea("ram", kRAM_BaseAddr, &_ram);\n'
        '  simctrl.RegisterExtension(&_memutil);\n\n',
        "",
    )
    start_marker = "std::string SimpleSystem::GetIsaString() const {"
    end_marker = "int SimpleSystem::Setup"
    start = text.find(start_marker)
    end = text.find(end_marker)
    if start == -1 or end == -1 or end < start:
        raise SystemExit("could not patch Ibex simple-system ISA string helper")
    replacement = """std::string SimpleSystem::GetIsaString() const {
  return "rv32imc";
}

"""
    text = text[:start] + replacement + text[end:]
    old_finish = """bool SimpleSystem::Finish() {
  VerilatorSimCtrl &simctrl = VerilatorSimCtrl::GetInstance();

  if (!simctrl.WasSimulationSuccessful()) {
    return false;
  }

  // Set the scope to the root scope, the ibex_pcount_string function otherwise
  // doesn't know the scope itself. Could be moved to ibex_pcount_string, but
  // would require a way to set the scope name from here, similar to MemUtil.
  svSetScope(svGetScopeFromName("TOP.ibex_simple_system"));

  std::cout << "\\nPerformance Counters" << std::endl
            << "====================" << std::endl;
  std::cout << ibex_pcount_string(false);

  std::ofstream pcount_csv("ibex_simple_system_pcount.csv");
  pcount_csv << ibex_pcount_string(true);

  return true;
}
"""
    new_finish = """bool SimpleSystem::Finish() {
  VerilatorSimCtrl &simctrl = VerilatorSimCtrl::GetInstance();

  if (!simctrl.WasSimulationSuccessful()) {
    return false;
  }

  std::cout << "\\nNaja Netlist Progress Counters" << std::endl
            << "==============================" << std::endl;

  std::ifstream progress("naja_ibex_progress.log");
  std::ofstream progress_csv("ibex_simple_system_pcount.csv");
  progress_csv << "counter,value\\n";
  std::string line;
  bool saw_progress = false;
  while (std::getline(progress, line)) {
    saw_progress = true;
    std::cout << line << std::endl;
    const std::string::size_type separator = line.find('=');
    if (separator != std::string::npos) {
      progress_csv << line.substr(0, separator) << ","
                   << line.substr(separator + 1) << "\\n";
    }
  }
  if (!saw_progress) {
    std::cout << "naja_ibex_progress.log was not produced" << std::endl;
  }

  return true;
}
"""
    if old_finish not in text:
        raise SystemExit("could not patch Ibex simple-system Finish() counter reporting")
    text = text.replace(old_finish, new_finish)
    target_path = work_dir / "ibex_simple_system_naja.cc"
    target_path.write_text(text, encoding="utf-8")
    return target_path


def patched_simple_system_header(repo_dir: Path, work_dir: Path) -> Path:
    source_path = repo_dir / "examples" / "simple_system" / "ibex_simple_system.h"
    text = source_path.read_text(encoding="utf-8")
    text = text.replace('#include "verilator_memutil.h"\n', "")
    text = text.replace("  VerilatorMemUtil _memutil;\n", "")
    text = text.replace("  MemArea _ram;\n", "")
    target_path = work_dir / "ibex_simple_system.h"
    target_path.write_text(text, encoding="utf-8")
    return target_path


def patched_simple_system_main(repo_dir: Path, work_dir: Path) -> Path:
    source_path = repo_dir / "examples" / "simple_system" / "ibex_simple_system_main.cc"
    target_path = work_dir / "ibex_simple_system_main.cc"
    target_path.write_text(source_path.read_text(encoding="utf-8"), encoding="utf-8")
    return target_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--primitives", type=Path, required=True)
    parser.add_argument("--max-cycles", default="200000")
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    repo_dir = args.repo.resolve()
    artifacts_dir = args.artifacts.resolve()
    generated_path = args.generated.resolve()
    primitives_path = args.primitives.resolve()
    check_generated_netlist(generated_path)
    work_dir = artifacts_dir / "local_sim" / "ibex_simple_system"
    obj_dir = work_dir / "obj"
    work_dir.mkdir(parents=True, exist_ok=True)

    tool_prefix = find_riscv_tool_prefix()
    patch_ibex_common_csr_asm(repo_dir)
    hello_dir = repo_dir / "examples" / "sw" / "simple_system" / "hello_test"
    run([
        "make",
        "-C",
        str(hello_dir),
        "hello_test.elf",
        f"CC={tool_prefix}gcc",
        f"OBJCOPY={tool_prefix}objcopy",
        "ARCH=rv32imc",
    ])
    firmware = repo_dir / "examples" / "sw" / "simple_system" / "hello_test" / "hello_test.vmem"
    firmware_bin = hello_dir / "hello_test.bin"
    run([
        tool_prefix + "objcopy",
        "-O", "binary",
        str(hello_dir / "hello_test.elf"),
        str(firmware_bin),
    ])
    write_word_vmem(firmware_bin, firmware, base_address=0)
    if not firmware.exists():
        raise SystemExit(f"missing Ibex hello_test firmware: {firmware}")

    vc_path = next((repo_dir / "build" / "fusesoc").glob("**/lint-verilator/*.vc"))
    patched_system = patched_simple_system(repo_dir, work_dir)
    patched_system_cpp = patched_simple_system_cpp(repo_dir, work_dir)
    patched_simple_system_header(repo_dir, work_dir)
    patched_system_main = patched_simple_system_main(repo_dir, work_dir)
    rtl_args = normalize_vc(vc_path, generated_path, primitives_path)
    rtl_args.extend([
        str(repo_dir / "vendor" / "lowrisc_ip" / "ip" / "prim_generic" / "rtl" / "prim_ram_2p_pkg.sv"),
        str(repo_dir / "vendor" / "lowrisc_ip" / "ip" / "prim_generic" / "rtl" / "prim_ram_2p.sv"),
        str(repo_dir / "shared" / "rtl" / "bus.sv"),
        str(repo_dir / "shared" / "rtl" / "ram_2p.sv"),
        str(repo_dir / "shared" / "rtl" / "sim" / "simulator_ctrl.sv"),
        str(repo_dir / "shared" / "rtl" / "timer.sv"),
        str(patched_system),
    ])

    include_dirs = [
        work_dir,
        repo_dir / "examples" / "simple_system",
        repo_dir / "vendor" / "lowrisc_ip" / "dv" / "verilator" / "cpp",
        repo_dir / "vendor" / "lowrisc_ip" / "dv" / "verilator" / "simutil_verilator" / "cpp",
    ]
    cflags = " ".join(["-std=c++17", "-DTOPLEVEL_NAME=ibex_simple_system"] +
                      [f"-I{path}" for path in include_dirs])
    cpp_sources = [
        patched_system_main,
        patched_system_cpp,
        repo_dir / "vendor" / "lowrisc_ip" / "dv" / "verilator" / "simutil_verilator" / "cpp" / "verilated_toplevel.cc",
        repo_dir / "vendor" / "lowrisc_ip" / "dv" / "verilator" / "simutil_verilator" / "cpp" / "verilator_sim_ctrl.cc",
    ]
    ldflags = "-pthread -lutil"
    command = [
        "verilator",
        "--cc",
        "--exe",
        "--build",
        "--timing",
        "--sv",
        "--top-module",
        "ibex_simple_system",
        "-Mdir",
        str(obj_dir),
        "-CFLAGS",
        cflags,
        "-LDFLAGS",
        ldflags,
        "-Wno-PINMISSING",
        "-Wno-TIMESCALEMOD",
        "--unroll-count",
        "72",
        f'-GSRAMInitFile="{firmware}"',
        *rtl_args,
        *(str(source) for source in cpp_sources),
    ]
    run(command, cwd=repo_dir)

    executable = obj_dir / "Vibex_simple_system"
    sim_args = [str(executable), f"--term-after-cycles={args.max_cycles}"]
    if args.debug:
        sim_args.append("+naja_debug=1")
    run(sim_args, cwd=work_dir)
    log_path = work_dir / "ibex_simple_system.log"
    log_text = log_path.read_text(encoding="utf-8", errors="replace") if log_path.exists() else ""
    expected_output = "Hello simple system"
    if expected_output not in log_text:
        progress_path = work_dir / "naja_ibex_progress.log"
        if progress_path.exists():
            progress_text = progress_path.read_text(encoding="utf-8", errors="replace").strip()
            raise SystemExit(
                f"Ibex simple-system did not print {expected_output!r}. "
                f"Naja netlist progress counters: {progress_text}"
            )
        raise SystemExit(f"Ibex simple-system did not print {expected_output!r}.")
    print("IBEX_LOCAL_SIM_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
