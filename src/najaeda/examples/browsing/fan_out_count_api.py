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
    
    max_fanout = netlist.get_max_fanout()
    print("max_fanout",max_fanout)
    print(max_fanout[0])
    for terms in max_fanout[1]:
        print(terms)
        print("Fanout for terminal", terms[0],":")
        for attr in terms[0].get_instance().get_attributes():
                print("   ", attr)
        for t in terms[1]:
            print(t)
            for attr in t.get_instance().get_attributes():
                print("   ", attr)