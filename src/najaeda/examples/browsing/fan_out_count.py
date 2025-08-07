# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
 
from os import path
import sys
import logging
from collections import deque
import time

from najaeda import netlist
import faulthandler

logging.basicConfig(level=logging.INFO)

if __name__ == '__main__':
    faulthandler.enable()
    # Load design
    netlist.load_primitives('xilinx')

    instances = set()
    benchmarks = path.join('..', '..', 'benchmarks')
    top = netlist.load_verilog(path.join(benchmarks, 'verilog', 'vexriscv.v'))
    
    drivers = list(top.get_flat_input_terms())
    for leaf in top.get_leaf_children():
        for term in leaf.get_flat_output_terms():
            drivers.append(term)
    
    max_fanout = 0
    for term in drivers:
        fanout = sum(1 for i in term.get_flat_fanout())
        if fanout > max_fanout:
            max_fanout = fanout
            
    print(max_fanout)