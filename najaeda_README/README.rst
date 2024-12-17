Naja EDA Python Package
=======================

Naja EDA is a Python package that provides data structures and APIs for developing post-synthesis Electronic Design Automation (EDA) algorithms.

Naja EDA provides a powerful yet simple framework designed to help software
and hardware developers efficiently navigate and manipulate electronic
design automation (EDA) workflows.

With Naja EDA, you can:

* Explore Netlists with Ease:

  * Navigate netlist hierarchy and connectivity effortlessly.
  * Browse at multiple levels of detail:

    * Bit-level or bus-level granularity.
    * Instance-by-instance exploration or flattened views at the primitives level.
    * Localized per-instance connections or comprehensive equipotential views.

* Perform ECO (Engineering Change Order) Transformations:

  * Seamlessly apply and manage changes to your designs.

* Prototype EDA Ideas Quickly:

  * Use an intuitive API to experiment with new EDA concepts and workflows.

* Develop Custom EDA Tools:

  * Build fast, tailored tools for solving specific challenges without relying on costly, proprietary EDA software.

Naja EDA empowers developers to innovate, adapt, and accelerate their EDA
processes with minimal overhead.

Naja EDA is the Python counterpart of the `Naja C++ project <https://github.com/najaeda/naja>`_.

Installation
------------

Install Naja EDA using pip:

.. code-block:: bash

    pip install najaeda

Examples
--------

Load a design from a liberty file and a Verilog file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Following snippet shows how to load primitive cells from a liberty file and
a netlist from a Verilog file.
.. snippet:: load_design_liberty

Load a design with pre-existing libraries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In FPGA design environments, Liberty files are often unavailable.

To address this, the following example demonstrates how to load primitives
without relying on Liberty files.

Naja EDA comes with pre-configured libraries to simplify this process.
Currently, it includes support for partial Xilinx primitives, but this can be
easily extended in the future. Don't hesitate to reach out if you need help.
.. snippet:: load_xilinx_design

Print all the instances in the netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Next example shows how to browse all the netlist and print all its content recursively.
.. snippet:: print_design_recursive

Similar to the previous example, but utilizing an instance visitor.
This approach allows you to perform operations on each instance while
also defining conditions for stopping or continuing exploration.
.. snippet:: print_design_visitor

Counting the Number of Leaves in a Netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The instance visitor provides a tool for collecting various types of information
about a netlist.

The following example demonstrates how to use the visitor’s callback
function to transmit user-defined arguments, allowing for flexible data processing.

This specific use case shows how to count the number of leaf instances in a netlist.
.. snippet:: count_leaves

Documentation
-------------
Naja EDA is a work in progress, and the documentation is still under development.

Naja documentation is available on the `Naja GitHub repository <https://github.com/najaeda/naja>`_.

Support
-------
If you encounter any issues or have questions, please report them on the
`Naja issue tracker <https://github.com/najaeda/naja/issues>`_.

License
-------
This project is licensed under the Apache License 2.0. \
See the `LICENSE <https://github.com/najaeda/naja/blob/main/LICENSE>`_ file for details.