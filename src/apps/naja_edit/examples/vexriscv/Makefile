include ../Makefile.inc

all: vexriscv.demo.GenFull_snl.v primitives.v

verilator: vexriscv.demo.GenFull_snl.v primitives.v
	${VERILATOR} --top-module vexriscv.demo.GenFull --lint-only primitives.v vexriscv.demo.GenFull_snl.v 

vexriscv.demo.GenFull_netlist.v: src/vexriscv.demo.GenFull.v
	${YOSYS} src/synth.ys

vexriscv.demo.GenFull_snl.v primitives.v: vexriscv.demo.GenFull_netlist.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -f verilog -t verilog -p ${PRIMITIVES} -i $< -o $@ -d primitives.v

clean:
	-rm vexriscv.demo.GenFull_netlist.v vexriscv.demo.GenFull_snl.v
