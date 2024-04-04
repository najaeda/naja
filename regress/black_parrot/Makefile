include ../Makefile.inc

all: parse_snl 

black_parrot_snl/snl.mf: black_parrot.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${PYTHON_ENV} ${NAJA_EDIT} -f verilog -t snl -i $< -o black_parrot_snl -p ${NANGATE45_PRIMITIVES}

parse_snl: black_parrot_snl/snl.mf
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${PYTHON_ENV} ${NAJA_EDIT} -f snl -i black_parrot_snl

clean:
	-rm black_parrot/*
