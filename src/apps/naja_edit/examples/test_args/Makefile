include ../Makefile.inc

all: top_out.v

top_out.v: top.v module1.v module2.v
	export PYTHONPATH=${SET_PYTHONPATH}; \
	${NAJA_EDIT} -f verilog -t verilog -i top.v -i module1.v -i module2.v -p ${PRIMITIVES} -o top_out.v

clean:
	-rm top_out.v
