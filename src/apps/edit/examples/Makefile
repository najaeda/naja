YOSYS ?= yosys
NAJA_EDIT ?= ${NAJA_INSTALL}/bin/naja_edit
PRIMITIVES ?= ${NAJA_INSTALL}/shared/primitives/xilinx.py
SNL_INTERCHANGE_DIR ?= ${NAJA_INSTALL}/shared/snlif
PYTHON_ENV ?= export PYTHONPATH=${NAJA_INSTALL}/lib/python;

all: addaccu_snl.v addaccu_edited.v

addaccu_snl_interface.txt: addaccu_snl/db_interface.snl
	capnp convert packed:text ${SNL_INTERCHANGE_DIR}/snl_interface.capnp DBInterface < $< > $@

addaccu_snl_implementation.txt: addaccu_snl/db_implementation.snl
	capnp convert packed:text ${SNL_INTERCHANGE_DIR}/snl_implementation.capnp DBImplementation < $< > $@

addaccu_netlist.v: addaccu.v
	${YOSYS} synth.ys

addaccu_snl: addaccu_netlist.v
	${PYTHON_ENV} ${NAJA_EDIT} -a verilog -b snl -i $< -o $@ -p ${PRIMITIVES}

addaccu_snl.v: addaccu_snl
	${PYTHON_ENV} ${NAJA_EDIT} -a snl -b verilog -i $< -o $@ 
	#${PYTHON_ENV} ${NAJA_EDIT} -a snl -b verilog -i $< -o $@ -e edit.py

clean:
	-rm -f edit.log addaccu_edited.v addaccu_snl.v
