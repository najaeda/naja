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

``Net``, ``Term`` and ``Instance`` provide the allocation-free
``has_source_references()`` presence check. It checks for a source declaration
or source origins, matching the explicit reference getters.
Instance checks apply to the instantiation; query its model explicitly for a
model definition. Concatenated nets return ``False``.

These classes also provide ``get_source_declaration()`` and
``get_source_origins()``. The first returns an immutable ``SourceReference``
or ``None``; the second returns a list in insertion order with exact duplicates
removed. ``SourceReference.range`` is a ``SourceRange`` (``file``, ``line``,
``end_line``, ``column``, ``end_column``). Optional ``provider``, ``language`` and
``representation`` strings are ``None`` when unknown. Paths are opaque; language
and representation are never inferred from extensions. Coordinates are one-based
byte positions with inclusive endpoints; zero means unknown. Lines and columns
support values through 4,294,967,295. ``get_source_range()`` also preserves these
wider columns without changing its return shape. NajaIF snapshots preserve the
full coordinate width, declarations, origins, and qualifiers. Verilog legacy
source attributes also support the full coordinate width.

An instance declaration identifies its instantiation in the loaded input.
``Instance.get_model_source_declaration()`` explicitly queries the model
definition (for the top instance, its top design). Origins identify upstream
inputs, potentially several RTL locations for a synthesized gate. No retained
text or AST is needed. Concatenated nets have no declaration or origins.

``get_source_range()`` continues to return the independent ``SourceLoc`` range.
It does not fall back to declarations or origins. SystemVerilog loading also
populates declarations for module definitions,
ports, named signals and source instantiations. Lowered logic receives origins
with provider ``slang``, language ``systemverilog`` and representation ``rtl``.
These queries work with ``keep_ast_link=False``. Structural Verilog loading
populates declarations and can decode Yosys origins explicitly; see :doc:`loading`.
VHDL population remains pending. Python source access is read-only.

New references support in-memory access, cloning, and NajaIF snapshot round trips.
The snapshot schema revision is owned by naja-if; incompatible revisions and
producer identities are rejected before loading payloads. Earlier snapshots must
be regenerated. Verilog export does not encode the new declarations and origins as attributes.
Use NajaIF to preserve these references. Source loading and queries are available
for viewer integration, including naja-schematic. Existing raw Verilog attributes
and legacy source-attribute export retain their established behavior.

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

``VerilogConfig.yosys_source_attributes`` defaults to ``False``. Set it to
``True`` to interpret Yosys ``src`` attributes as upstream origins without
inferring a language, representation, or AST link. Structural Verilog now
populates declarations independently of this option; see :doc:`loading`.
