Term Class
==========

Term Overview
-------------

In **najaeda**, a :py:class:`najaeda.netlist.Term` (also referred to as a Port in Verilog terminology)
can represent the following scenarios:

1. **Single Bit Scalar Terminal**: A terminal representing a single scalar signal. See :py:meth:`najaeda.netlist.Term.is_scalar` .
2. **Full Bus Terminal**: A terminal representing an entire bus. See :py:meth:`najaeda.netlist.Term.is_bus` .
3. **Single Bus Bit Terminal**: A terminal representing a single bit of a bus. See :py:meth:`najaeda.netlist.Term.is_bus_bit` .

Term Attributes
---------------

.. autoclass:: najaeda.netlist.Term
    :members:
    :undoc-members:
    :show-inheritance: