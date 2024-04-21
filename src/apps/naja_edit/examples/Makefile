YOSYS ?= yosys
NAJA_EDIT ?= ${NAJA_INSTALL}/bin/naja_edit
PRIMITIVES ?= ${NAJA_INSTALL}/shared/primitives/xilinx.py
PYTHON_ENV ?= export PYTHONPATH=${NAJA_INSTALL}/lib/python;

all: addaccu_edited.v

addaccu_netlist.v: addaccu.v
	${YOSYS} synth.ys

addaccu_edited.v: addaccu_netlist.v
	${PYTHON_ENV} ${NAJA_EDIT} -a verilog -b verilog -i $< -o addaccu_naja.v -p ${PRIMITIVES} -e edit.py
