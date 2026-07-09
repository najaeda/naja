Net Class
=========

Net Overview
------------

In **najaeda**, a :py:class:`najaeda.netlist.Net` can represent the following scenarios:

1. **Simple Scalar Net**: A net that represents a single scalar signal. See :py:meth:`najaeda.netlist.Net.is_scalar`.
2. **Full Bus Net**: A net that encompasses an entire bus. See :py:meth:`najaeda.netlist.Net.is_bus`.
3. **Single Bit of a Bus**: A net that corresponds to an individual bit within a bus. See :py:meth:`najaeda.netlist.Net.is_bus_bit`.
4. **Concatenation of Bits**: A net created by combining multiple bits. See :py:meth:`najaeda.netlist.Net.is_concat`.

Use nets for local connectivity inside a design model.  Use
:class:`najaeda.netlist.Equipotential` when the question crosses hierarchy
boundaries.

Common tasks
------------

.. code-block:: python

   net = instance.get_net("data")
   print(net.get_name(), net.get_width())

   for term in net.get_terms():
       print(term)

   net.set_name("renamed_data")

Net Attributes
--------------

.. autoclass:: najaeda.netlist.Net
    :members:
    :undoc-members:
    :show-inheritance:
    :no-index:
