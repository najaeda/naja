# Naja Python API Examples

Naja Python API allows users to:

- browse the netlist data, collect informations, ...
- Apply ECO type transformations.

To use the Python API in `naja_edit`, start by creating a Python script containing an `edit` function.

```python
from naja import snl

def edit():
    universe = snl.SNLUniverse.get()
    top = universe.getTopDesign()

    # Do something with 'top'
```

## Print All Design Content

The following script recursively browses the design and prints instances, terminals, nets, and their connectivity.

```python
from naja import snl

def print_instance_tree(design):
    for ins in design.getInstances():
        print(f"Instance: {ins.getName()}")
        model = ins.getModel()
        for term in design.getTerms():
            print(f"  Terminal: {term}")
        for net in design.getNets():
            print(f"  Net: {net}")
            for bit in net.getBits():
                for component in bit.getComponents():
                    print(f"    Component: {component}")
        print_instance_tree(model)

def edit():
    universe = snl.SNLUniverse.get()
    top = universe.getTopDesign()

    print_instance_tree(top)
```

## Remove Interface Buffers from an FPGA Design

The following script removes interface buffers `'IBUF'`, `'OBUF'`, and `'BUFG'` from a design synthesized for an FPGA.

```python
from naja import snl

def delete_io_bufs(design):
    for ins in design.getInstances():
        model = ins.getModel()
        if model.isPrimitive():
            model_name = model.getName()
            if model_name in ['IBUF', 'BUFG']:
                input_net = ins.getInstTerm(model.getScalarTerm('I')).getNet()
                output_net = ins.getInstTerm(model.getScalarTerm('O')).getNet()
                for component in output_net.getComponents():
                    component.setNet(input_net)
                output_net.destroy()
                ins.destroy()
            elif model_name == 'OBUF':
                input_net = ins.getInstTerm(model.getScalarTerm('I')).getNet()
                output_net = ins.getInstTerm(model.getScalarTerm('O')).getNet()
                for component in input_net.getComponents():
                    component.setNet(output_net)
                input_net.destroy()
                ins.destroy()

def edit():
    universe = snl.SNLUniverse.get()
    top = universe.getTopDesign()

    delete_io_bufs(top)
```

## Browse all modules and display them based on their number of instances terms and nets

```python
from naja import snl
import pandas as pd
import matplotlib.pyplot as plt

def plot_design_stats(library):
    data_list = []
    for design in library.getDesigns():
        nb_terms = sum(1 for _ in design.getBitTerms())
        nb_nets = sum(1 for _ in design.getBitNets())
        nb_instances = sum(1 for _ in design.getInstances())
        data_list.append({
            'design': design.getName(),
            'nb_terms': nb_terms,
            'nb_nets': nb_nets,
            'nb_instances': nb_instances
        })
    pandas_data = pd.DataFrame(data_list).set_index('design')
    plot = pandas_data.plot.bar(y=['nb_terms', 'nb_nets', 'nb_instances'], stacked=True)

    # Set title and labels
    plot.set_title('Design Statistics', fontsize=16, fontweight='bold')
    plot.set_xlabel('Design Name', fontsize=12)
    plot.set_ylabel('Count', fontsize=12)

    plot_figure = plot.get_figure()
    plot_figure.tight_layout()
    plot_figure.savefig('design_stats.png')


def edit():
    universe = snl.SNLUniverse.get()
    topDesign = universe.getTopDesign()
    topLibrary = topDesign.getLibrary() #top library contains top design

    plot_design_stats(topLibrary)
```

This script generates a plot of design statistics, as illustrated in the image below:
![Design Stats](./images/design_stats.png)
