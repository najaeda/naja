# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

BENCHMARKS := simple up_counter param_inst binary_ops_supported byte_ports

.PHONY: sim clean $(BENCHMARKS)

sim: $(BENCHMARKS)

$(BENCHMARKS):
	$(MAKE) -C $@ sim

clean:
	@for d in $(BENCHMARKS); do $(MAKE) -C $$d clean; done
