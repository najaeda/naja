# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# --- Tool paths (set NAJA_INSTALL before including this file) ---
NAJA_EDIT  ?= $(NAJA_INSTALL)/bin/naja_edit
PRIMITIVES ?= $(NAJA_INSTALL)/share/naja/primitives

# --- naja-regress source ---
# Set NAJA_REGRESS_PATH to a local checkout to skip network fetches.
# If unset, subtrees are fetched from GitHub.
NAJA_REGRESS_PATH ?=
NAJA_REGRESS_REPO ?= https://github.com/najaeda/naja-regress.git
NAJA_REGRESS_REF  ?= main

# --- fetch_subtree function ---
# Usage inside a recipe:
#   $(call fetch_subtree,<subpath-in-naja-regress>,<local-dest-dir>)
#
# If NAJA_REGRESS_PATH is set the subtree is rsynced from the local clone.
# Otherwise a minimal sparse-checkout is performed from GitHub.
define fetch_subtree
	@set -e; \
	if [ -n "$(NAJA_REGRESS_PATH)" ]; then \
		echo "[fetch] rsync $(NAJA_REGRESS_PATH)/$(1)/ -> $(2)"; \
		mkdir -p "$(2)"; \
		rsync -a --delete "$(NAJA_REGRESS_PATH)/$(1)/" "$(2)/"; \
	else \
		echo "[fetch] clone $(NAJA_REGRESS_REPO) $(1) -> $(2)"; \
		_tmp=$$(mktemp -d); \
		git clone --depth=1 -b "$(NAJA_REGRESS_REF)" "$(NAJA_REGRESS_REPO)" "$$_tmp"; \
		mkdir -p "$(2)"; \
		rsync -a "$$_tmp/$(1)/" "$(2)/"; \
		rm -rf "$$_tmp"; \
	fi
endef
