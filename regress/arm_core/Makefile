include ../Makefile.inc

all: verif.gold verilator.gold verif.error

verif.error: arm_core_snl_error.v primitives.v 
	${YOSYS} src/comp_error.ys
	touch $@

verif.gold: arm_core_snl.v primitives.v 
	${YOSYS} src/comp_gold.ys
	touch $@

eqy.gold: arm_core_snl.v primitives.v 
	${EQY} src/comp.eqy
	touch $@

verilator.gold: arm_core_snl.v primitives.v
	${VERILATOR} --top-module arm_core --lint-only primitives.v arm_core_snl.v
	touch $@

arm_core_netlist.v: src/arm_core.v
	${YOSYS} src/synth.ys

arm_core_snl.v primitives.v: arm_core_netlist.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -f verilog -t verilog -p ${PRIMITIVES} -i arm_core_netlist.v -o arm_core_snl.v -d primitives.v

arm_core_snl_error.v: arm_core_netlist.v src/add_error.py
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -e src/add_error.py -f verilog -t verilog -p ${PRIMITIVES} -i arm_core_netlist.v -o arm_core_snl_error.v -d primitives.v

clean:
	-rm arm_core_snl.v primitives.v arm_core_netlist.v 
