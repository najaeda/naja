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


# snippet-start: dle
def apply_dle(top, keep_attributes=None):
    # Trace back from design outputs
    visited = set()
    traced_terms = list(top.get_flat_output_terms())
    for leaf in top.get_leaf_children():
        attributes =  list(leaf.get_attributes())
        for attr in attributes:
            if attr in keep_attributes:
                for term in leaf.get_flat_input_terms():
                    traced_terms.append(term)
                break
    for termToTrace in traced_terms:
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

    to_delete = [leaf for leaf in top.get_leaf_children() if leaf not in instances]
    for leaf in to_delete:
        leaf.delete()
    return to_delete
# snippet-end: dle

if __name__ == '__main__':
    faulthandler.enable()
    # Load design
    netlist.load_primitives('xilinx')
    instances = set()
    benchmarks = path.join('..', 'benchmarks')
    top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'vexriscv.v')])
    attributes =  ['DONT_TOUCH', 'KEEP', 'preserve', 'noprune']
    nb_deleted = apply_dle(top, attributes)
    
    top.dump_verilog("./", "result.v")
    print(f'deleted {len(nb_deleted)} leaves')