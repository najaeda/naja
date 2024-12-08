Naja EDA Python Package
=======================

Naja EDA is a Python package that provides data structures and APIs for developing post-synthesis Electronic Design Automation (EDA) algorithms. It serves as the Python counterpart to the `Naja C++ project <https://github.com/najaeda/naja>`_.

Features
--------

- **Netlist Simplification**: Perform constant propagation and dead logic elimination.
- **Logic Replication**: Facilitate duplication of logic elements for optimization.
- **Netlist Partitioning**: Divide netlists into manageable sections.
- **Place and Route Support**: Assist in ASIC and FPGA design flows.

Installation
------------

Install Naja EDA using pip:

.. code-block:: bash

    pip install najaeda

Examples
--------
Following snippet shows how to load primitive cells from a liberty file and
a netlist from a Verilog file.
.. snippet:: load_design

Next example shows how to browse all the netlist and print all its content.
.. snippet:: print_design

Documentation
-------------

Comprehensive documentation is available on the `Naja GitHub repository <https://github.com/najaeda/naja>`_.

License
-------

This project is licensed under the Apache License 2.0. See the `LICENSE <https://github.com/najaeda/naja/blob/main/LICENSE>`_ file for details.