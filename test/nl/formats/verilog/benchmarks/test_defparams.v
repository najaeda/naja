// defparam with escape ids
//

module ins_decode ();
wire N_261 ;
wire [7:7] decodes_in_0_0_a3_0_3;
wire [7:7] decodes_in_0_0_1_0_Z;
wire [11:5] inst ;
wire N_24 ;
wire N_38 ;
wire N_73 ;
wire N_75 ;
wire N_74 ;
wire N_33 ;
wire N_23_i ;
wire GND ;
wire VCC ;

// @7:114
  CFG4 \decodes_in_0_0_1_0[7]  (
	.A(N_261),
	.B(decodes_in_0_0_a3_0_3[7]),
	.C(inst[8]),
	.D(N_24),
	.Y(decodes_in_0_0_1_0_Z[7])
);
defparam \decodes_in_0_0_1_0[7] .INIT=16'h5054;
// @7:114
  CFG2 \decodes_in_0_a2_i_o3[8]  (
	.A(inst[9]),
	.B(inst[10]),
	.Y(N_38)
);
// @7:58
  CFG4 \decodes_RNO[6]  (
	.A(N_73),
	.B(N_75),
	.C(N_74),
	.D(N_33),
	.Y(N_23_i)
);
defparam \decodes_RNO[6] .INIT=16'h0001;
  GND GND_Z (
	.Y(GND)
);
  VCC VCC_Z (
	.Y(VCC)
);
endmodule /* ins_decode */