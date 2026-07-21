High-Level API Reference
========================

This is the recommended Python API for scripts and applications.  It wraps
the native SNL database with path-aware Python objects and provides safer
editing behavior for hierarchical designs.

``Net.Type.ASSIGNX`` and ``Net.Type.ASSIGNZ`` preserve structural X and Z
constants.  Query them with ``Net.is_constx()`` and ``Net.is_constz()``;
``Net.is_const()`` covers all four constant values.

For the lower-level compiled extension module, see :doc:`raw_api`.

.. automodule:: najaeda.netlist
    :members:
    :undoc-members:
    :show-inheritance:
