YOSYS ?= yosys
SNL_X2Y ?= ../../build/src/snl/snippets/cpp/snl_vrl_snippet
PRIMITIVES ?= ../../primitives/xilinx.py 

all: adder_snl.v primitives.v
#	${YOSYS} src/comp.ys

adder_netlist.v: src/adder.v
	${YOSYS} src/synth.ys

adder_snl.v primitives.v: adder_netlist.v
	${SNL_X2Y} -p ${PRIMITIVES} -i adder_netlist.v -o adder_snl.v -d primitives.v

clean:
	-rm adder_snl.v primitives.v adder_netlist.v 
