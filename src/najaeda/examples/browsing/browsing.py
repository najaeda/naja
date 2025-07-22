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
    
    instance = top.get_child_instance("IBusCachedPlugin_cache")
    
    for term in instance.get_terms():
        if term.is_bus():
            print("Bus Term: ", term.get_name(), " with width: ", term.get_width())
        else:
            print("Scalar Term: ", term.get_name())
    for child in instance.get_child_instances():
        if child.is_assign():
            continue
        print("Child instance: ", child.get_name(), " of model ", child.get_model_name())