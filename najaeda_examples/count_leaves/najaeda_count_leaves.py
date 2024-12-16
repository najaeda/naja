# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# snippet-start: load_design

from os import path
import sys

from najaeda import netlist
from najaeda import instance_visitor

netlist.load_primitives('xilinx')
benchmarks = path.join('..','benchmarks')
top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'arm_core_netlist.v')])

# snippet-start: count_leaves
leaves = {"count": 0}
def count_leaves(instance, leaves):
    if instance.is_leaf():
        leaves["count"] += 1
visitor_config = instance_visitor.VisitorConfig(callback=count_leaves, args=(leaves,))
instance_visitor.Visitor(top).visit(top, visitor_config)
print(f"{top} nb leaves={leaves['count']}")
# snippet-end: count_leaves

sys.exit(0)
