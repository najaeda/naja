include ../Makefile.inc

all: edges.list verif.gold verilator.gold 

#verif.error: arm_core_snl_error.v primitives.v 
#	${YOSYS} src/comp_error.ys
#	touch $@

verif.gold: arm_core_snl.v primitives.v 
	${YOSYS} src/comp_gold.ys
	touch $@

eqy.gold: arm_core_snl.v primitives.v 
	-rm -rf comp
	${EQY} src/comp.eqy
	touch $@

verilator.gold: arm_core_snl.v primitives.v
	${VERILATOR} --top-module arm_core --lint-only -Wno-PINMISSING primitives.v arm_core_snl.v
	touch $@

arm_core_netlist.v: src/arm_core.v
	${YOSYS} src/synth.ys

arm_core_snl/snl.mf: arm_core_netlist.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${PYTHON_ENV} ${NAJA_EDIT} -f verilog -t snl -i $< -o arm_core_snl -p ${PRIMITIVES}

gui: arm_core_snl/snl.mf
	export PYTHONPATH=${SET_PYTHONPATH}; \
	../../../../../build/src/gui/naja_gui -f snl -i arm_core_snl

edges.list: arm_core_snl/snl.mf gen_edge_list.py
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -f snl -i arm_core_snl -e gen_edge_list.py

arm_core_snl.v primitives.v: arm_core_netlist.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -f verilog -t verilog -p ${PRIMITIVES} -i arm_core_netlist.v -o arm_core_snl.v -d primitives.v

arm_core_snl_error.v: arm_core_netlist.v ../scripts/plot_designs_stats.py
	export PYTHONPATH=${SET_PYTHONPATH}:./venv/lib/python3.12/site-packages; \
	${NAJA_EDIT} -e ../scripts/plot_designs_stats.py -f verilog -p ${PRIMITIVES} -i arm_core_netlist.v

clean:
	-rm arm_core_snl.v primitives.v arm_core_netlist.v 
