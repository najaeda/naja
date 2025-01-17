Introduction
============
**najaeda** is a an Electronic Design Automation (EDA) Python package
that provides open source data structures and APIs for the development
of post logic synthesis EDA algorithms
such as: netlist simplification (constant and dead logic propagation),
logic replication, netlist partitioning, ASIC and FPGA place and route, ...

**najaeda** provides a powerful yet simple framework designed
to help software **AND** hardware developers efficiently navigate and
manipulate electronic design automation (EDA) workflows.

With **najaeda**, it is possible to:

* **Explore Netlists with Ease**:
    * Navigate netlist hierarchy and connectivity effortlessly.
    * Browse at multiple levels of detail:
        * Bit-level or bus-level granularity.
        * Instance-by-instance exploration or flattened views at the primitives level.
        * Localized per-instance connections or comprehensive equipotential views.
* **Perform ECO (Engineering Change Order) Transformations**:
    * Seamlessly apply and manage changes to your designs.
* **Prototype EDA Ideas Quickly**:
    * Use an intuitive API to experiment with new EDA concepts and workflows.
* **Develop Custom EDA Tools**:
    * Build fast, tailored tools for solving specific challenges
    without relying on costly, proprietary EDA software.

**najaeda** empowers developers to innovate, adapt, and accelerate
their EDA processes with minimal overhead.

Information about the **najaeda** PyPI package is available at https://pypi.org/project/najaeda .

If you want more details about the underlying **naja** C++ library,
please visit the **naja** GitHub repository at https://github.com/najaeda/naja .

Installation
------------
**najaeda** can be easily installed using pip:

.. code-block:: bash
    
    pip install najaeda

Bug Reports
-----------
Please report any bugs to the `najaeda` issue tracker at
https://github.com/najaeda/naja/issues .

License
-------
This project is licensed under the Apache License 2.0.
See the `LICENSE <https://github.com/najaeda/naja/blob/main/LICENSE>`_ file for details.