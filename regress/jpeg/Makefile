include ../Makefile.inc

all: jpeg_snl/snl.mf  

jpeg_snl/snl.mf: jpeg.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${PYTHON_ENV} ${NAJA_EDIT} -f verilog -t snl -i $< -o jpeg_encoder_snl -p ${ASAP7_PRIMITIVES}

clean:
	-rm arm_core_snl.v primitives.v arm_core_netlist.v 
