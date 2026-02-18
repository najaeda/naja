////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_append_assign_different_output_bus();
wire [1:0] source_bus;
wire [1:0] sink_bus_a;
wire [1:0] sink_bus_b;

assign sink_bus_a[1] = source_bus[1];

assign sink_bus_b[0] = source_bus[0];
endmodule //top_append_assign_different_output_bus
