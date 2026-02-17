////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_bus_assign_non_contiguous();
wire [5:0] source_bus;
wire [5:0] sink_bus;

assign sink_bus[5:4] = source_bus[5:4];

assign sink_bus[2:0] = source_bus[2:0];
endmodule //top_bus_assign_non_contiguous
