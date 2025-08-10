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

    benchmarks = path.join('..', '..', 'benchmarks')
    top = netlist.load_verilog(path.join(benchmarks, 'verilog', 'vexriscv.v'))
    
    start = time.time()
    
    trace_back_terms = list(top.get_flat_output_terms())
    for leaf in top.get_leaf_children():
        if leaf.is_sequential():
            for term in leaf.get_flat_input_terms():
                if term.is_sequential():
                    trace_back_terms.append(term)
    
    global_max_logic_level = 0
    path2ll = {}
    for termToTrace in trace_back_terms:
        queue = deque([(termToTrace, 0)])
        max_logic_level = 0
        visited = set() # To avoid cycles
        while len(queue) > 0:
            term, logic_level = queue.popleft()
            path2ll[(tuple(term.pathIDs), tuple(term.termIDs))] = logic_level
            if term in visited:
                continue
            visited.add(term)
            if logic_level > max_logic_level:
                max_logic_level = logic_level
            leaf_drivers = term.get_equipotential().get_leaf_drivers()
            for driver in leaf_drivers:
                if driver.is_sequential() or driver.get_instance().is_top():
                    continue
                instance = driver.get_instance()
                key = (tuple(driver.pathIDs), tuple(driver.termIDs))
                if key in path2ll and path2ll[key] > logic_level + 1:
                    continue
                input_terms = instance.get_combinatorial_inputs(driver)
                for input_term in input_terms:
                    queue.append((input_term, logic_level + 1))
        if max_logic_level > global_max_logic_level:
            global_max_logic_level = max_logic_level
    
    print(global_max_logic_level)
    end = time.time()
    print('LL count done in', end - start, 'seconds')
    
    
    