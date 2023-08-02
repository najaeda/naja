YOSYS ?= yosys
VERILATOR ?= verilator
SNL_X2Y ?= ../../build/src/snl/snippets/cpp/snl_vrl_snippet
PRIMITIVES ?= ../../primitives/xilinx.py 

all: adder_snl.v primitives.v verilator

verilator: adder_snl.v primitives.v
	${VERILATOR} --top-module adder --lint-only primitives.v adder_snl.v

adder_netlist.v: src/adder.v
	${YOSYS} src/synth.ys

adder_snl.v primitives.v: adder_netlist.v
	${SNL_X2Y} -a verilog -b verilog -p ${PRIMITIVES} -i $< -o adder_snl.v -d primitives.v

clean:
	-rm adder_snl.v primitives.v adder_netlist.v 
