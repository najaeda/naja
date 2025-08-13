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
    
    trace_back_terms = list(top.get_output_bit_terms())
    print("Number of top terms to trace back: ", len(trace_back_terms))
    num_leaves = sum(1 for _ in top.get_leaf_children())
    print("Number of leaves: ", num_leaves)
    for leaf in top.get_leaf_children():
        if leaf.is_sequential():
            for term in leaf.get_input_bit_terms():
                if term.is_sequential():
                    trace_back_terms.append(term)
    
    global_max_logic_level = 0
    path2ll = {}
    print("Number of leaf terms to trace back: ", len(trace_back_terms))
    for termToTrace in trace_back_terms:
        queue = deque([(termToTrace, [])])
        max_logic_level = 0
        while len(queue) > 0:
            term, path = queue.popleft()
            if path is not None and len(path) > max_logic_level:
                max_logic_level = len(path)
            leaf_drivers = term.get_equipotential().get_leaf_drivers()
            for driver in leaf_drivers:
                if driver.is_sequential() or driver.get_instance().is_top():
                    continue
                instance = driver.get_instance()
                key = (tuple(driver.pathIDs), tuple(driver.termIDs))
                if key in path2ll and path2ll[key] > len(path):
                    continue
                if path is not None:
                    path2ll[(tuple(driver.pathIDs), tuple(driver.termIDs))] = len(path)
                input_terms = instance.get_combinatorial_inputs(driver)
                for input_term in input_terms:
                    if path is not None and input_term in path:
                        continue # Avoid cycles
                    newPath = []
                    if path is not None:
                        newPath = path.copy()
                    newPath.append(input_term)
                    queue.append((input_term, newPath))        
        if max_logic_level > global_max_logic_level:
            global_max_logic_level = max_logic_level
    
    print(global_max_logic_level)
    end = time.time()
    print('LL count done in', end - start, 'seconds')
    
    
    
