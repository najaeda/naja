Term Class
==========

Term Overview
-------------

In **najaeda**, a :py:class:`najaeda.netlist.Term` (also referred to as a Port in Verilog terminology)
can represent the following scenarios:

1. **Single Bit Scalar Terminal**: A terminal representing a single scalar signal. See :py:meth:`najaeda.netlist.Term.is_scalar` .
2. **Full Bus Terminal**: A terminal representing an entire bus. See :py:meth:`najaeda.netlist.Term.is_bus` .
3. **Single Bus Bit Terminal**: A terminal representing a single bit of a bus. See :py:meth:`najaeda.netlist.Term.is_bus_bit` .

Terms are used for both top-level ports and instance terminals.  A term can
answer local connectivity through its connected net, or flat connectivity
through an equipotential.

Common tasks
------------

.. code-block:: python

   clk = instance.get_term("clk")
   print(clk.get_direction())

   net = clk.get_net()
   equi = clk.get_equipotential()

Term Attributes
---------------

.. autoclass:: najaeda.netlist.Term
    :members:
    :undoc-members:
    :show-inheritance:
    :no-index:
