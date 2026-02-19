# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

BENCHMARKS := simple up_counter param_inst binary_ops_supported byte_ports \
	seq_add_out_plus_one seq_add_one_plus_out seq_binary_non_add \
	seq_add_non_increment seq_rhs_unresolved seq_rhs_width_mismatch \
	seq_rhs_direct_match

.PHONY: sim clean $(BENCHMARKS)

sim: $(BENCHMARKS)

$(BENCHMARKS):
	$(MAKE) -C $@ sim

clean:
	@for d in $(BENCHMARKS); do $(MAKE) -C $$d clean; done
