# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
 
from os import path
import sys
from collections import deque

from najaeda import netlist
from najaeda import snl
import faulthandler

if __name__ == '__main__':
    faulthandler.enable()
    # Load design
    netlist.load_primitives('xilinx')
    instances = set()
    benchmarks = path.join('..', 'benchmarks')
    top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'arm_core_netlist.v')])
    
    # snippet-start: dle
    # Trace back from design outputs
    visited = set()
    output_terms = top.get_flat_output_terms()
    for termToTrace in output_terms:
        queue = deque([termToTrace])
        while queue:
            term = queue.popleft()
            if term in visited:
                continue
            visited.add(term)
            equipotential = term.get_equipotential()
            leaf_drivers = equipotential.get_leaf_drivers()
            for driver in leaf_drivers:
                instance = driver.get_instance()
                instances.add(instance)
                input_terms = instance.get_flat_input_terms()
                queue.extend(input_terms)
    
    leaf_children = top.get_leaf_children()
    to_delete = [leaf for leaf in leaf_children if leaf not in instances]
    for leaf in to_delete:
        leaf.delete()
    # snippet-end: dle
    
    top.dump_verilog("./", "result.v")
    print("deleted", len(to_delete))