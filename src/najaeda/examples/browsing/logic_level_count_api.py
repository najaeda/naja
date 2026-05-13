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
    
    logging.info(f"Logic level count {netlist.get_max_logic_level()[0]}")
    
    end = time.time()
    logging.info(f"LL count done in {end - start} seconds")
    
    # print the max logic level paths
    max_level_paths = netlist.get_max_logic_level()[1]
    for path in max_level_paths:
        for term in path:
            logging.info(f"{term}")
            for attr in term.get_instance().get_attributes():
                logging.info(f"   {attr}")
