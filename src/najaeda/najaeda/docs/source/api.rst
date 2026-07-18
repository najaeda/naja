High-Level API Reference
========================

This is the recommended Python API for scripts and applications.  It wraps
the native SNL database with path-aware Python objects and provides safer
editing behavior for hierarchical designs.

State values use :class:`najaeda.netlist.LogicValue` and
:class:`najaeda.netlist.LogicVector`. ``Net.set_constant()`` creates explicit
typed constant-driver topology. Sequential and memory instances expose
explicit power-up state through ``has_init()`` and ``get_init_value()``;
``get_reset_value()`` is independent from power-up initialization.
Child-instance traversal is lossless by default; category-specific regular,
assign-glue, constant-driver, and helper iterators avoid repeated filtering.

For the lower-level compiled extension module, see :doc:`raw_api`.

.. automodule:: najaeda.netlist
    :members:
    :undoc-members:
    :show-inheritance:
