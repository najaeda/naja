include ../Makefile.inc

all: arm_core_snl.v primitives.v verilator
	${YOSYS} src/comp.ys

verilator: arm_core_snl.v primitives.v
	${VERILATOR} --top-module arm_core --lint-only primitives.v arm_core_snl.v

arm_core_netlist.v: src/arm_core.v
	${YOSYS} src/synth.ys

arm_core_snl.v primitives.v: arm_core_netlist.v
	${NAJA_EDIT} -f verilog -t verilog -p ${PRIMITIVES} -i arm_core_netlist.v -o arm_core_snl.v -d primitives.v

clean:
	-rm arm_core_snl.v primitives.v arm_core_netlist.v 
