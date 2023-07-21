YOSYS ?= yosys
VERILATOR ?= verilator
SNL_X2Y ?= ../../build/src/snl/snippets/cpp/snl_vrl_snippet
PRIMITIVES ?= ../../primitives/xilinx.py 

all: arm_core_snl.v primitives.v verilator
	${YOSYS} src/comp.ys

verilator: arm_core_snl.v primitives.v
	${VERILATOR} --top-module arm_core --lint-only primitives.v arm_core_snl.v

arm_core_netlist.v: src/arm_core.v
	${YOSYS} src/synth.ys

arm_core_snl.v primitives.v: arm_core_netlist.v
	${SNL_X2Y} -a verilog -b verilog -p ${PRIMITIVES} -i arm_core_netlist.v -o arm_core_snl.v -d primitives.v

clean:
	-rm arm_core_snl.v primitives.v arm_core_netlist.v 
