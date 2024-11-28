# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from os import path
from najaeda import netlist

def print_netlist(instance):
    for child_instance in instance.get_child_instances():
        print(f"{child_instance}:{child_instance.get_model_name()}")
        print_netlist(child_instance)

benchmarks = path.join('..','benchmarks')
liberty_files = ['NangateOpenCellLibrary_typical.lib', 'fakeram45_1024x32.lib', 'fakeram45_64x32.lib']
liberty_files = list(map(lambda p:path.join(benchmarks, 'liberty', p), liberty_files))
    
netlist.load_liberty(liberty_files)
top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'tinyrocket.v')])

print_netlist(top)
