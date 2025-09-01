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
from najaeda import instance_visitor
import faulthandler

logging.basicConfig(level=logging.INFO)

def check_leaves(instance):
    if instance.is_leaf():
        if not instance.has_modeling():
            if instance.is_primitive():
                logging.info(f"Primitive instance without modeling: {instance} {instance.get_model_name()}")
            elif instance.is_blackbox():
                logging.info(f"Leaf blackbox instance without modeling: {instance} {instance.get_model_name()}")
            else:
                logging.info(f"Leaf instance without modeling: {instance} {instance.get_model_name()}")


if __name__ == '__main__':
    faulthandler.enable()
    # Load design
    netlist.load_primitives('xilinx')

    benchmarks = path.join('..', '..', 'benchmarks')
    top = netlist.load_verilog(path.join(benchmarks, 'verilog', 'vexriscv.v'))
    
    start = time.time()

    visitor_config = instance_visitor.VisitorConfig(callback=check_leaves)
    instance_visitor.visit(top, visitor_config)
    
    end = time.time()
    
