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

Source information
------------------

``Net``, ``Term`` and ``Instance`` provide ``get_source_declaration()`` and
``get_source_origins()``. The first returns an immutable ``SourceReference``
or ``None``; the second returns a list in insertion order with exact duplicates
removed. ``SourceReference.range`` is a ``SourceRange`` (``file``, ``line``,
``end_line``, ``column``, ``end_column``). Optional ``provider``, ``language`` and
``representation`` strings are ``None`` when unknown. Paths are opaque; language
and representation are never inferred from extensions. Coordinates are one-based
byte positions with inclusive endpoints; zero means unknown.

An instance declaration identifies its instantiation in the loaded input.
``Instance.get_model_source_declaration()`` explicitly queries the model
definition (for the top instance, its top design). Origins identify upstream
inputs, potentially several RTL locations for a synthesized gate. No retained
text or AST is needed. Concatenated nets have no declaration or origins.

``get_source_range()`` is unchanged: it reads the independent legacy location
slot. Existing loaders have not yet migrated to the new roles, so the new
queries return missing values until populated explicitly through the raw API.
Expert scripts can use the raw setters in :doc:`raw_api`; high-level editing
wrappers are deferred until frontend workflows establish their need.

New references currently support in-memory access and cloning. NajaIF export
and Verilog export with metadata enabled reject them until typed persistence is
implemented; discard output after a failed export. Verilog export with metadata
disabled intentionally omits source metadata. Legacy-only export is unchanged.

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
