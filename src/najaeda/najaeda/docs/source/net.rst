Net Class
=========

.. automodule:: najaeda.netlist.Net
    :members:
    :undoc-members:
    :show-inheritance:

Net Overview
------------

In **najaeda**, a **Net** can represent the following scenarios:

1. **Simple Scalar Net**: A net that represents a single scalar signal. See :py:meth:`najaeda.netlist.Net.is_scalar`.
2. **Full Bus Net**: A net that encompasses an entire bus. See :py:meth:`najaeda.netlist.Net.is_bus`.
3. **Single Bit of a Bus**: A net that corresponds to an individual bit within a bus. See :py:meth:`najaeda.netlist.Net.is_bus_bit`.
4. **Concatenation of Bits**: A net created by combining multiple bits. See :py:meth:`najaeda.netlist.Net.is_concat`.

Net Attributes
--------------

.. autoclass:: najaeda.netlist.Net
    :members:
    :undoc-members:
    :show-inheritance: