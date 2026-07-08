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

Generic Gates (and, or, etc.)
-----------------------------

**najaeda** supports **generic gates** as defined in Verilog, specifically:

- **n-input gates**: `and`, `nand`, `or`, `nor`, `xor`, `xnor`
- **n-output gates**: `buf`, `not`

In this model:

- **n-input gates** have a **single scalar output** and a **bus input terminal** (of size *n*).
- **n-output gates** have a **scalar input** and a **bus output terminal** (of size *n*).

All terminals in these generic instances are **unnamed** (see :py:attr:`najaeda.netlist.Instance.is_unnamed`).

Instance Attributes
-------------------

.. autoclass:: najaeda.netlist.Instance
    :members:
    :undoc-members:
    :show-inheritance:
    :no-index:
