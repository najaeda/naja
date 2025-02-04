# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# snippet-start: load_design

from os import path
import sys
import logging

from najaeda import netlist
from najaeda import instance_visitor

logging.basicConfig(level=logging.INFO)

netlist.load_primitives('xilinx')
benchmarks = path.join('..','benchmarks')
top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'vexriscv.v')])

# snippet-start: count_leaves
leaves = {"count": 0, "assigns": 0, "constants": 0}
def count_leaves(instance, leaves):
    if instance.is_leaf():
        if instance.is_assign():
            leaves["assigns"] += 1
        elif instance.is_const():
            leaves["constants"] += 1
        else:
            leaves["count"] += 1
visitor_config = instance_visitor.VisitorConfig(callback=count_leaves, args=(leaves,))
instance_visitor.visit(top, visitor_config)
print(f"{top} leaves count")
print(f"nb_assigns={leaves['assigns']}")
print(f"nb constants={leaves['constants']}")
print(f"nb other leaves={leaves['count']}")
# snippet-end: count_leaves

sys.exit(0)
