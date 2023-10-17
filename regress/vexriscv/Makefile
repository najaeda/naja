YOSYS ?= yosys
VERILATOR ?= verilator
SNL_X2Y ?= ../../build/src/snl/snippets/cpp/snl_vrl_snippet
PRIMITIVES ?= ../../primitives/xilinx.py 

all: vexriscv.demo.GenFull_snl.v primitives.v
#	yosys src/comp.ys

verilator: vexriscv.demo.GenFull_snl.v primitives.v
	${VERILATOR} --top-module vexriscv.demo.GenFull --lint-only primitives.v vexriscv.demo.GenFull_snl.v 

vexriscv.demo.GenFull_netlist.v: src/vexriscv.demo.GenFull.v
	${YOSYS} src/synth.ys

vexriscv.demo.GenFull_snl.v primitives.v: vexriscv.demo.GenFull_netlist.v
	${SNL_X2Y} -a verilog -b verilog -p ${PRIMITIVES} -i $< -o $@ -d primitives.v

clean:
	-rm vexriscv.demo.GenFull_netlist.v vexriscv.demo.GenFull_snl.v
