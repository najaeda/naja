Naja EDA Python Package
=======================

Naja EDA is a Python package that provides data structures and APIs for developing post-synthesis Electronic Design Automation (EDA) algorithms.
It serves as the Python counterpart to the `Naja C++ project <https://github.com/najaeda/naja>`_.

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

najaeda comes with pre-configured libraries to simplify this process.
Currently, it includes support for partial Xilinx primitives, but this can be
easily extended in the future. Don't hesitate to reach out if you need help.
.. snippet:: load_xilinx_design

Print all the instances in the netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Next example shows how to browse all the netlist and print all its content.
.. snippet:: print_design

Documentation
-------------
najaeda is a work in progress, and the documentation is still under development.

Naja documentation is available on the `Naja GitHub repository <https://github.com/najaeda/naja>`_.

Support
-------
Please put up issues on the Delocate issue tracker.
If you encounter any issues or have questions, please report them on the
`Naja issue tracker <https://github.com/najaeda/naja/issues>`_.

License
-------
This project is licensed under the Apache License 2.0.
See the `LICENSE <https://github.com/najaeda/naja/blob/main/LICENSE>`_ file for details.