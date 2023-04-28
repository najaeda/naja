all: arm_core_snl.v primitives.v
	yosys src/comp.ys

arm_core_netlist.v: src/arm_core.v
	yosys src/synth.ys

arm_core_snl.v primitives.v: arm_core_netlist.v
	../../build/src/snl/snippets/cpp/snl_vrl_snippet -p ../../primitives/xilinx.py -i arm_core_netlist.v -o arm_core_snl.v -d primitives.v