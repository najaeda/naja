Equipotential Class
===================

Equipotential Overview
----------------------

The :class:`najaeda.netlist.Equipotential` class represents a flat connected
component across hierarchy.  It is the right tool when local net connectivity
is not enough.

An equipotential can enumerate:

* top-level terms connected to the signal;
* instance terms connected through hierarchy;
* leaf drivers and readers;
* constant drivers.

Example
-------

.. code-block:: python

   equi = top.get_term("clk").get_equipotential()

   for reader in equi.get_leaf_readers():
       print(reader)

Equipotential Attributes
------------------------

.. autoclass:: najaeda.netlist.Equipotential
    :members:
    :undoc-members:
    :show-inheritance:
    :no-index:
