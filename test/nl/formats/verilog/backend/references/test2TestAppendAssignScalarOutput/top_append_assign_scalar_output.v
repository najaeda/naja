////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_append_assign_scalar_output();
wire [1:0] source_bus;
wire [1:0] sink_bus;
wire scalar_sink;

assign sink_bus[1] = source_bus[1];

assign scalar_sink = source_bus[0];
endmodule //top_append_assign_scalar_output
