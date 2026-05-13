////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_assigns_before_gate();
wire [1:0] source_bus;
wire [1:0] sink_bus;
wire gate_out;

assign sink_bus = source_bus;

and and2(gate_out, sink_bus[0], sink_bus[1]);
endmodule //top_assigns_before_gate
