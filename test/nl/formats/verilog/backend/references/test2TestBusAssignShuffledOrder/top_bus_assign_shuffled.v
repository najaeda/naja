////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_bus_assign_shuffled();
wire [3:0] source_bus;
wire [3:0] sink_bus;

assign sink_bus[3] = source_bus[3];

assign sink_bus[2:1] = source_bus[2:1];

assign sink_bus[0] = source_bus[0];
endmodule //top_bus_assign_shuffled
