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

    start = time.time()
    print('Starting DLE')

    instances = set()
    benchmarks = path.join('..', '..', 'benchmarks')
    top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'vexriscv.v')])
    attributes =  ['DONT_TOUCH', 'KEEP', 'preserve', 'noprune']
    netlist.apply_dle()
    
    end = time.time()
    print('DLE done in', end - start, 'seconds')