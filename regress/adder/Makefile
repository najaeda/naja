include ../Makefile.inc

all: adder_snl.v primitives.v verilator

verilator: adder_snl.v primitives.v
	${VERILATOR} --top-module adder --lint-only primitives.v adder_snl.v

adder_netlist.v: src/adder.v
	${YOSYS} src/synth.ys

adder_snl.v primitives.v: adder_netlist.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -f verilog -t verilog -p ${PRIMITIVES} -i $< -o adder_snl.v -d primitives.v

clean:
	-rm adder_snl.v primitives.v adder_netlist.v 
