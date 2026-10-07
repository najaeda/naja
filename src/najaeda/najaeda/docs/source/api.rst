High-Level API Reference
========================

This is the recommended Python API for scripts and applications.  It wraps
the native SNL database with path-aware Python objects and provides safer
editing behavior for hierarchical designs.

``Net.Type.ASSIGNX`` and ``Net.Type.ASSIGNZ`` preserve structural X and Z
constants.  Query them with ``Net.is_constx()`` and ``Net.is_constz()``;
``Net.is_const()`` covers all four constant values.

For the lower-level compiled extension module, see :doc:`raw_api`.
For the optional ``najaeda.schematic`` viewer API, see :doc:`schematic`.

Term roles
----------

Scalar terms and individual bus bits provide ``get_role()`` and
``get_reset_active_level()``, returning raw ``SNLTermRole`` and
``SNLActiveLevel`` values. ``is_clock()``, ``is_async_reset()``,
``is_async_set()``, ``is_sync_reset()``, ``is_sync_set()``, ``is_reset()``,
``is_enable()``, ``is_data_input()``, ``is_data_output()``, and ``is_data()``
classify the resolved role. ``is_reset()`` includes reset roles only;
use the set predicates for set pins. Whole-bus queries raise ``ValueError``;
use ``term.get_bit(index)`` or iterate ``term.get_bits()``.

Instance terms resolve parameter-dependent roles in their path context;
top-level terms use static roles. Configure vendor models with the raw
``SNLDesign.setRolesFromParameters()`` API described in :doc:`raw_api`.
For example, ``ff.get_term("R").is_reset()`` returns false when the selected
configuration explicitly assigns no role to ``R``. ``get_role()`` then
returns ``SNLTermRole.Other``.

.. automodule:: najaeda.netlist
    :members:
    :undoc-members:
    :show-inheritance:
