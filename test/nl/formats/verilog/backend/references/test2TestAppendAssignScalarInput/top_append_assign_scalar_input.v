////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_append_assign_scalar_input();
wire [1:0] source_bus;
wire [1:0] sink_bus;
wire scalar_in;

assign sink_bus[1] = source_bus[1];

assign sink_bus[0] = scalar_in;
endmodule //top_append_assign_scalar_input
