# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from os import path
import sys
import logging

from najaeda import netlist
from najaeda import instance_visitor

logging.basicConfig(level=logging.INFO)

benchmarks = path.join('..', '..', 'benchmarks')
liberty_files = [
    'NangateOpenCellLibrary_typical.lib',
    'fakeram45_1024x32.lib',
    'fakeram45_64x32.lib'
]
liberty_files = list(map(lambda p:path.join(benchmarks, 'liberty', p), liberty_files))
    
netlist.load_liberty(liberty_files)
top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'tinyrocket.v')])

# snippet-start: print_design_visitor
def print_instance(instance):
    print(f"{instance}:{instance.get_model_name()}")
visitor_config = instance_visitor.VisitorConfig(callback=print_instance)
instance_visitor.visit(top, visitor_config)
# snippet-end: print_design_visitor

sys.exit(0)