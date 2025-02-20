# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from os import path
import sys
import logging

from najaeda import netlist

logging.basicConfig(level=logging.INFO)

# snippet-start: load_xilinx_design
netlist.load_primitives('xilinx')
benchmarks = path.join('..','benchmarks')
top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'vexriscv.v')])
# snippet-end: load_xilinx_design
#print instances under top
for instance in top.get_child_instances():
    print(f"{instance}:{instance.get_model_name()}")

sys.exit(0)