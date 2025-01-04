# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# snippet-start: load_design

from os import path
import sys

from najaeda import netlist
from najaeda import snl

if __name__ == '__main__':
    
    # Load design
    netlist.load_primitives('xilinx')
    instances = set()
    benchmarks = path.join('..','benchmarks')
    top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'arm_core_netlist.v')])
    
    # Trace back from design outputs
    visited = set()
    for termToTrace in top.get_flat_output_terms(): 
        queue = []
        queue.append(termToTrace)
        while queue:
            term = queue.pop(0)
            if term in visited:
                continue
            visited.add(term)
            for driver in term.get_equipotential().get_leaf_drivers():
                instances.add(driver.get_instance())
                for reader in driver.get_instance().get_flat_input_terms():
                    queue.append(reader)
    count = 0
    toDelete = []
    for leaf in top.get_leaf_children():
        if not leaf in instances: 
            toDelete.append(leaf)
            count += 1
    for leaf in toDelete:
        leaf.delete()
    top.dump_verilog("./", "result.v")
    print("deleted ",count)