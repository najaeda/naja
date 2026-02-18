////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module unnamed_instance_model(input i, output o);
endmodule //unnamed_instance_model

module top_unnamed_instance_name();
wire input_net;
wire output_net;

unnamed_instance_model instance_0 (
  .i(input_net),
  .o(output_net)
);
endmodule //top_unnamed_instance_name
