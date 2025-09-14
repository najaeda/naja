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
from najaeda import naja
import faulthandler

logging.basicConfig(level=logging.INFO)

if __name__ == '__main__':
    faulthandler.enable()
    # Load design
    netlist.load_primitives('xilinx')

    benchmarks = path.join('..', '..', 'benchmarks')
    top = netlist.load_verilog(path.join(benchmarks, 'verilog', 'vexriscv.v'))
    
    start = time.time()
    
    print("Logic level count ", netlist.get_max_logic_level()[0])
    
    end = time.time()
    print('LL count done in', end - start, 'seconds')
    
    # print the max logic level paths
    max_level_paths = netlist.get_max_logic_level()[1]
    i = 0
    for p in max_level_paths:
        print("Path", i, ":")
        i += 1
        for n in p:
            component = n.getComponent()
            if isinstance(component, naja.SNLInstTerm):
                component = component.getBitTerm()
            term = netlist.Term(n.getPath().getPathIDs(), component)
            print("Term:", term)
            #if len(term.get_instance().pathIDs) > 0 and term.get_instance().count_attributes() > 0:
            print("Attributes:")
            for att in term.get_instance().get_attributes():
                print(att.get_name(), "=", att.get_value())
        
    
    