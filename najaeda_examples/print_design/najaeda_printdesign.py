from najaeda import netlist

def print_netlist(instance):
    for child_instance in instance.get_child_instances():
        print(f"{child_instance}:{child_instance.get_model_name()}")
        print_netlist(child_instance)

netlist.Netlist.load_liberty('cells.lib')
top = netlist.Netlist.load_verilog('design.v')

print_netlist(top)