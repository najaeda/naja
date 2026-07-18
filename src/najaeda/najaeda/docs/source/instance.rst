Instance Class
==============

Instance Overview
-----------------

In **najaeda**, an :py:class:`najaeda.netlist.Instance` encapsulates the concept
of an instance in its hierarchical context.

When an **Instance** is modified through editing methods,
**najaeda** will automatically manage the necessary uniquification.

Use instances to navigate hierarchy, inspect the model represented by an
occurrence, enumerate child instances, access terms and nets, and apply
localized edits.

Common tasks
------------

.. code-block:: python

   top = netlist.get_top()

   for child in top.get_child_instances():
       print(child.get_name(), child.get_model_name())

   alu = top.get_child_instance("u_alu")
   for leaf in alu.get_leaf_instances():
       print(leaf.get_name())

Choosing an instance view
-------------------------

Constant drivers and continuous-assignment glue are explicit instances in
SNL.  :py:meth:`najaeda.netlist.Instance.get_child_instances` is therefore
lossless: it returns regular instances and both kinds of implementation helper.
Use the named views instead of repeating ad-hoc filters:

.. code-block:: python

   regular = list(top.get_regular_child_instances())
   assigns = list(top.get_assign_child_instances())
   constants = list(top.get_constant_driver_child_instances())
   helpers = list(top.get_helper_child_instances())

The categories form these invariants:

* all children = regular + assign glue + constant drivers;
* helper children = assign glue + constant drivers.

An instance for which :py:meth:`~najaeda.netlist.Instance.is_constant_driver`
is true is not assign glue, even when its constant-driver kind is ``assign``.
That kind records HDL provenance.  Use
:py:meth:`~najaeda.netlist.Instance.is_assign` only for assign-glue instances,
and :py:meth:`~najaeda.netlist.Instance.is_regular` for neither category.

Generic Gates (and, or, etc.)
-----------------------------

**najaeda** supports **generic gates** as defined in Verilog, specifically:

- **n-input gates**: `and`, `nand`, `or`, `nor`, `xor`, `xnor`
- **n-output gates**: `buf`, `not`

In this model:

- **n-input gates** have a **single scalar output** and a **bus input terminal** (of size *n*).
- **n-output gates** have a **scalar input** and a **bus output terminal** (of size *n*).

All terminals in these generic instances are **unnamed** (see :py:attr:`najaeda.netlist.Instance.is_unnamed`).

Use :py:meth:`najaeda.netlist.Instance.is_and`, :py:meth:`~najaeda.netlist.Instance.is_nand`,
:py:meth:`~najaeda.netlist.Instance.is_or`, :py:meth:`~najaeda.netlist.Instance.is_nor`,
:py:meth:`~najaeda.netlist.Instance.is_xor`, and :py:meth:`~najaeda.netlist.Instance.is_xnor`
to identify n-input gate families without decoding raw truth-table masks.

Instance Attributes
-------------------

.. autoclass:: najaeda.netlist.Instance
    :members:
    :undoc-members:
    :show-inheritance:
    :no-index:
