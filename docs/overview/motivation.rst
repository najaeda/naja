Why Naja?
---------

Naja is an EDA (Electronic Design Automation) project aiming at
offering open source data structures and APIs for the development of
post logic synthesis EDA algorithms such as:
netlist simplification (constant and dead logic propagation),
logic replication, netlist partitioning, ASIC and FPGA place and route, ...

In most EDA flows, data exchange is done by using standard netlist formats
(Verilog, LEF/DEF, EDIF, …) which were not designed to represent data structures
content with high fidelity.

To overcome this problem, Naja relies on Cap'n Proto open source interchange format.

Naja also emphasizes EDA applications parallelization
(targeting in particular cloud computing) by providing a robust object
identification mechanism allowing to partition and merge data across the network.

The core of Naja is formed by two interrelated data structures:
the Structured Netlist (SNL) and the Dissolved Netlist (DNL - Under development).
SNL is tailored for high-fidelity representation of hierarchical netlists,
while DNL offers a flattened netlist view, optimized for rapid,
multi-threaded analysis and optimization tool development.

.. image:: images/Naja-Architecture.png
   :alt: Naja Architecture