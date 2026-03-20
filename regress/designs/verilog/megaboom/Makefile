# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Root of the naja3 regress tree (3 levels up from this Makefile).
REGRESS_ROOT := $(abspath $(CURDIR)/../../..)

include $(REGRESS_ROOT)/Makefile.inc

# --- Paths resolved from REGRESS_ROOT ---
LIBERTY_DIR := $(REGRESS_ROOT)/liberty/asap7
SOURCES_DIR := $(CURDIR)/sources

# Subtree paths inside naja-regress
_REGRESS_LIBERTY_SUBPATH := liberty/asap7
_REGRESS_SOURCES_SUBPATH := designs/verilog/megaboom/sources

# --- Liberty input list (asap7) ---
LIBERTY_INPUTS = \
	asap7sc7p5t_AO_LVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_AO_SLVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_AO_SRAM_FF_nldm_211120.lib.gz \
	asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz \
	asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz \
	asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz \
	asap7sc7p5t_INVBUF_SRAM_FF_nldm_220122.lib.gz \
	asap7sc7p5t_OA_LVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_OA_SLVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_OA_SRAM_FF_nldm_211120.lib.gz \
	asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib \
	asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib \
	asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib \
	asap7sc7p5t_SEQ_SRAM_FF_nldm_220123.lib \
	asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_SIMPLE_SLVT_FF_nldm_211120.lib.gz \
	asap7sc7p5t_SIMPLE_SRAM_FF_nldm_211120.lib.gz

LIBERTY_FULL  = $(foreach f,$(LIBERTY_INPUTS),$(LIBERTY_DIR)/$(f))
PARSE_LIBERTY = -p $(LIBERTY_FULL)

# --- Verilog input list ---
INPUTS = \
	megaboom1.v megaboom2.v megaboom3.v \
	data_2048x8.v btb_128x56.v ebtb_128x40.v meta_128x120.v \
	array_256x128.v lb_32x128.v sdq_17x64.v dataArrayB_256x64.v \
	tag_array_64x168.v tag_array_64x184.v l2_tlb_ram_0_512x46.v \
	regfile_128x65.v regfile_128x64.v table_128x44.v mem_256x4.v \
	table_256x48.v table_128x52.v

INPUTS_FULL  = $(foreach f,$(INPUTS),$(SOURCES_DIR)/$(f))
PARSE_INPUTS = $(foreach f,$(INPUTS_FULL),-i $(f))

# -----------------------------------------------------------------------
# Default target
# -----------------------------------------------------------------------
.PHONY: all fetch clean

all: megaboom_out.v

# -----------------------------------------------------------------------
# Fetch targets (idempotent via sentinel files)
# -----------------------------------------------------------------------

$(LIBERTY_DIR)/.fetched:
	$(call fetch_subtree,$(_REGRESS_LIBERTY_SUBPATH),$(LIBERTY_DIR))
	@touch $@

$(SOURCES_DIR)/.fetched:
	$(call fetch_subtree,$(_REGRESS_SOURCES_SUBPATH),$(SOURCES_DIR))
	@touch $@

fetch: $(LIBERTY_DIR)/.fetched $(SOURCES_DIR)/.fetched

# -----------------------------------------------------------------------
# Build targets
# -----------------------------------------------------------------------

megaboom_out.v: | $(LIBERTY_DIR)/.fetched $(SOURCES_DIR)/.fetched
	$(NAJA_EDIT) \
	    -f verilog -t verilog \
	    $(PARSE_LIBERTY) \
	    $(PARSE_INPUTS) \
	    -o megaboom_out.v

# -----------------------------------------------------------------------
clean:
	-rm -f megaboom_out.v
