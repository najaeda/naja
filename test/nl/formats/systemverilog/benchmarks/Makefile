# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

BENCHMARKS := simple up_counter param_inst binary_ops_supported byte_ports \
	implicit_width_ports port_directions non_ansi_ports model_reuse \
	unary_not unary_not_binary_supported unary_not_binary_unsupported \
	seq_add_out_plus_one seq_add_one_plus_out seq_enable_else_default \
	seq_enable_bus1_supported seq_preincrement_supported \
	seq_reset_only_supported seq_list_single_wrapper_supported \
	seq_nested_begin_wrapper_supported seq_rhs_direct_match \
	seq_binary_non_add seq_add_non_increment seq_rhs_unresolved \
	seq_rhs_width_mismatch

.PHONY: sim clean $(BENCHMARKS)

sim: $(BENCHMARKS)

$(BENCHMARKS):
	$(MAKE) -C $@ sim

clean:
	@for d in $(BENCHMARKS); do $(MAKE) -C $$d clean; done
