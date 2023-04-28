all: vexriscv.demo.GenFull_snl.v primitives.v
#	yosys src/comp.ys

vexriscv.demo.GenFull_netlist.v: src/vexriscv.demo.GenFull.v
	yosys src/synth.ys

vexriscv.demo.GenFull_snl.v primitives.v: vexriscv.demo.GenFull_netlist.v
	../../build/src/snl/snippets/cpp/snl_vrl_snippet -p ../../primitives/xilinx.py -i $< -o $@ -d primitives.v

clean:
	rm vexriscv.demo.GenFull_netlist.v vexriscv.demo.GenFull_snl.v
