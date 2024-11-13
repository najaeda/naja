YOSYS ?= yosys
NAJA_EDIT ?= ${NAJA_INSTALL}/bin/naja_edit
PRIMITIVES ?= ${NAJA_INSTALL}/share/naja/primitives/xilinx.py
SNL_INTERCHANGE_DIR ?= ${NAJA_INSTALL}/share/naja/snlif
PYTHON_ENV ?= export PYTHONPATH=${NAJA_INSTALL}/lib/python;

all: addaccu_snl.v addaccu_edited.v

addaccu_snl_interface.txt: addaccu_snl/snl.mf
	capnp convert packed:text ${SNL_INTERCHANGE_DIR}/snl_interface.capnp DBInterface < addaccu_snl/db_interface.snl > $@

addaccu_snl_implementation.txt: addaccu_snl/snl.mf
	capnp convert packed:text ${SNL_INTERCHANGE_DIR}/snl_implementation.capnp DBImplementation < addaccu_snl/db_implementation.snl > $@

yosys addaccu_netlist.v: addaccu.v
	${YOSYS} synth.ys

snl_if addaccu_snl/snl.mf: addaccu_netlist.v
	${PYTHON_ENV} ${NAJA_EDIT} -l -f verilog -t snl -i $< -o addaccu_snl -p ${PRIMITIVES}

addaccu_snl.v: addaccu_snl/snl.mf
	${PYTHON_ENV} ${NAJA_EDIT} -l -f snl -t verilog -i addaccu_snl -o $@ 

addaccu_edited.v: addaccu_snl/snl.mf
	${PYTHON_ENV} ${NAJA_EDIT} -l -f snl -t verilog -i addaccu_snl -o $@ -e edit.py

clean:
	-rm -rf __pycache__ addaccu_snl
	-rm -f naja_edit.log edit.log addaccu_netlist.v addaccu_edited.v addaccu_snl.v *.txt *~
