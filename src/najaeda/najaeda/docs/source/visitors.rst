najaeda visitors
================

Instance Visitor
----------------

Instance Visitor Overview
^^^^^^^^^^^^^^^^^^^^^^^^^

The instance visitor module provides a small recursive traversal helper over
the high-level :class:`najaeda.netlist.Instance` hierarchy.

.. code-block:: python

   from najaeda import instance_visitor

   def report(instance):
       print(instance.get_name(), instance.get_model_name())

   config = instance_visitor.VisitorConfig(callback=report)
   instance_visitor.visit(top, config)

.. autoclass:: najaeda.instance_visitor.VisitorConfig
    :members:
    :undoc-members:
    :show-inheritance:

.. autofunction:: najaeda.instance_visitor.visit
