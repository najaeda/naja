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

Traversing Assign Instances
---------------------------

By default, Assign instances remain leaf endpoints of an equipotential.  Use
``Equipotential.Mode.TRAVERSE_ASSIGNS`` to cross them and omit their instance
terminals from the result.  This is useful when Assigns should behave as
transparent signal connections:

.. code-block:: python

   from najaeda import netlist

   equi = top.get_term("clk").get_equipotential(
       mode=netlist.Equipotential.Mode.TRAVERSE_ASSIGNS)

The default is ``Equipotential.Mode.STANDARD``, so existing calls without a
mode continue to include Assign instance terminals.  Additional traversal
modes can be added to this enum without changing the constructor signature.

Equipotential Attributes
------------------------

.. autoclass:: najaeda.netlist.Equipotential
    :members:
    :undoc-members:
    :show-inheritance:
    :no-index:
