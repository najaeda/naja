////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top_append_assign_output_step_mismatch();
wire [3:0] source_bus;
wire [3:0] sink_bus;

assign sink_bus[3:2] = source_bus[3:2];

assign sink_bus[3] = source_bus[1];
endmodule //top_append_assign_output_step_mismatch
