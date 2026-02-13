BENCHMARKS := simple up_counter param_inst

.PHONY: sim clean $(BENCHMARKS)

sim: $(BENCHMARKS)

$(BENCHMARKS):
	$(MAKE) -C $@ sim

clean:
	@for d in $(BENCHMARKS); do $(MAKE) -C $$d clean; done
