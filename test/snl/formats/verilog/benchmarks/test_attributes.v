// Verilog netlist with attributes on all possible objects for unit testing

// Attribute for the entire module
(* MODULE_ATTRIBUTE = "Top level simple_netlist module", MODULE_VERSION = "1.0" *)
module simple_netlist (
    (* INPUT_ATTRIBUTE_A = "Input signal A" *)
    input a,        // Input a
    (* INPUT_ATTRIBUTE_B = "Input signal B" *)
    input b,        // Input b
    (* OUTPUT_ATTRIBUTE_AND = "Output of AND gate" *)
    output and_out, // Output of AND gate
    (* OUTPUT_ATTRIBUTE_OR = "Output of OR gate" *)
    output or_out   // Output of OR gate
);

    // Internal wire attributes for connecting sub-modules
    (* WIRE_ATTRIBUTE = "Wire connecting AND gate output to top output" *)
    wire and_wire;
    (* WIRE_ATTRIBUTE = "Wire connecting OR gate output to top output" *)
    wire or_wire;

    // Attribute for and2 gate instance
    (* INSTANCE_ATTRIBUTE_AND = "and2_inst", description = "2-input AND gate instance" *)
    and2 and2_inst (
        .a(a),
        .b(b),
        .y(and_wire)
    );

    // Attribute for or2 gate instance
    (* INSTANCE_ATTRIBUTE_OR = "or2_inst", description = "2-input OR gate instance" *)
    or2 or2_inst (
        .a(a),
        .b(b),
        .y(or_wire)
    );

    // Assign internal wires to outputs with attributes
    (* ASSIGN_ATTRIBUTE = "Connect AND gate output to top module output" *)
    assign and_out = and_wire;
    
    (* ASSIGN_ATTRIBUTE = "Connect OR gate output to top module output" *)
    assign or_out = or_wire;

endmodule

// Attribute for the and2 module
(* MODULE_ATTRIBUTE_AND2 = "2-input AND gate module", MODULE_VERSION = "1.0" *)
module and2 (
    (* INPUT_ATTRIBUTE = "AND gate input a" *)
    input a,
    (* INPUT_ATTRIBUTE = "AND gate input b" *)
    input b,
    (* OUTPUT_ATTRIBUTE = "AND gate output y" *)
    output y
);
endmodule

// Attribute for the or2 module
(* MODULE_ATTRIBUTE_OR2 = "2-input OR gate module", MODULE_VERSION = "1.0" *)
module or2 (
    (* INPUT_ATTRIBUTE = "OR gate input a" *)
    input a,
    (* INPUT_ATTRIBUTE = "OR gate input b" *)
    input b,
    (* OUTPUT_ATTRIBUTE = "OR gate output y" *)
    output y
);
endmodule