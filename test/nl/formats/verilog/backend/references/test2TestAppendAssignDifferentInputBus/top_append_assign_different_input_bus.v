////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_append_assign_different_input_bus();
wire [1:0] source_bus_a;
wire [1:0] source_bus_b;
wire [1:0] sink_bus;

assign sink_bus[1] = source_bus_a[1];

assign sink_bus[0] = source_bus_b[0];
endmodule //top_append_assign_different_input_bus
