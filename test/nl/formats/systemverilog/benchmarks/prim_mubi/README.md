<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

Upstream OpenTitan integration fixture, copied unchanged from
[lowRISC/opentitan commit e404f576d4e48cdbd2e56e4b34f686107bf38850](https://github.com/lowRISC/opentitan/tree/e404f576d4e48cdbd2e56e4b34f686107bf38850/hw/ip/prim/rtl).

`prim_mubi_pkg.sv` and its four assertion headers retain their upstream
copyright and Apache-2.0 notices. REUSE metadata is in `.reuse/dep5`.
Tests define `SYNTHESIS` to select the upstream dummy assertion macros.
No package functions or static package checks are replaced.

The value regression exhausts all 256 input pairs for `mubi4_and_hi`,
`mubi4_or_hi`, and `mubi4_test_true_loose(mubi4_and_hi(...))` from continuous
assignments and `always_comb`.
