Loading and Saving
==================

The top-level load and dump functions live in :mod:`najaeda.netlist`.

Session lifecycle
-----------------

.. code-block:: python

   from najaeda import netlist

   netlist.reset()
   top = netlist.create_top("top")

Call :func:`najaeda.netlist.reset` at the beginning of scripts and tests when
you need an empty universe.  Loading a design returns the top
:class:`najaeda.netlist.Instance` wrapper.

Verilog
-------

.. code-block:: python

   from najaeda import netlist

   config = netlist.VerilogConfig()
   top = netlist.load_verilog(["cells.v", "design.v"], config)

Use Verilog loading for structural Verilog netlists.  Load Liberty or
primitive libraries first when the design instantiates standard cells or
technology primitives.

Loader arguments are validated before native parsing starts.  A missing input
raises :class:`FileNotFoundError` with both the supplied and resolved path;
wrong argument and configuration types raise :class:`TypeError`; empty paths
or unsupported configuration values raise :class:`ValueError`.  Errors for a
list entry identify its zero-based index (for example, ``files[2]``).

SystemVerilog
-------------

.. code-block:: python

   from najaeda import netlist

   config = netlist.SystemVerilogConfig()
   top = netlist.load_system_verilog(["pkg.sv", "rtl.sv"], config)

SystemVerilog loading elaborates the design through the native frontend and
then builds SNL objects.  Frontend diagnostics are raised as native
``SystemVerilog*`` exceptions from :mod:`najaeda.naja`.

By default, an incremental diagnostics report is written to
``naja_sv_diagnostics.log``. Set ``diagnostics_report_path=None`` in
:class:`najaeda.netlist.SystemVerilogConfig` to disable that file and retain
console diagnostics only.

``SystemVerilogConfig`` validates path, boolean, define, and warning-suppression
fields when it is created and again when loading begins.  Entries in
``suppress_warnings`` are warning names such as ``"width-trunc"``; omit the
``-W`` or ``-Wno-`` command-line prefix.  To load only from a command file,
pass an empty file list and set ``flist``.

VHDL
----

.. code-block:: python

   from najaeda import netlist

   top = netlist.load_vhdl("design.vhd")

For a supported structural file containing multiple design units, select the
top entity explicitly:

.. code-block:: python

   top = netlist.load_vhdl("hierarchy.vhd", top="soc_top")

The RTL path can infer the top when exactly one entity is not instantiated by
another design unit, including instances inside nested generate statements.
If there are several roots or no root, pass ``top`` explicitly.

VHDL loading is experimental and implements a bounded, two-state RTL subset.
It supports integer generic defaults and explicit generic maps, constrained
arrays, static indexing and slices, nested ``for`` loops, ``for generate`` and
static ``if``/``elsif``/``else generate`` statements, component instances, vector registers, synchronous reset/enable,
asynchronous reset/set,
and immediate process variables. Generated instances can bind indexed or sliced
ports; integer constant tables can supply their generic values. Architecture
and package constants may use positional array aggregates, including
unconstrained array types with ``natural``, ``positive``, or ``integer`` indices.

Conditional generates select the first true branch at elaboration time, using
static boolean expressions built from integer generics, constants, enclosing
loop parameters, and supported static package functions. Only the selected
branch creates hardware. Generate bodies may contain assignments, instances,
clocked processes, and nested conditional or loop generates. Instance names
retain generate scope prefixes. Generate-local declarations, alternative labels,
and explicit branch-body ``end`` statements remain unsupported and are diagnosed.

Record types declared in packages or architectures support nested records,
arrays of records, field selection, whole-record and field assignments, and
positional or named record aggregates. Named aggregates must supply every field
exactly once; a sole ``others`` association is also supported when its value
matches every field type. Record ports are flattened to one SNL bus in declaration
order, recursively, with the first field at the most significant end. Original
vector directions determine the order of bits within each field. Record type
names remain distinct for assignment checking. ``std_ulogic`` and
``std_ulogic_vector`` use the same binary lowering as their resolved counterparts;
multiple drivers and nonbinary literals remain unsupported.

Pure package functions with static integer and boolean arguments can compute
constants, generic defaults, vector bounds, and constant expressions. The
supported bodies contain local scalar variables/constants, variable assignments,
``if``/``elsif``/``else``, bounded ``for`` loops, nested function calls, and early
``return`` statements. Positional and named actuals and default arguments are
supported. Defaults and function bodies resolve package declarations rather
than caller-local names. Integer arithmetic includes division, ``mod``, ``rem``,
exponentiation, and ``abs``; boolean comparisons and logical operators preserve
short-circuit evaluation. Subtype violations, overflow, missing returns,
ambiguous overloads, and excessive recursion or evaluation work are diagnosed.
Function calls with runtime arguments, vector/string function evaluation, and
impure functions remain unsupported. Array attributes and interface defaults
are retained by the parser, but their elaboration is not yet supported.

The NEORV32 default ``neorv32_top`` configuration is a development target, not yet
a supported load. Its complete package parses, and ``index_size_f`` and
``sel_natural_f`` can elaborate a test design. The complete core file list now
stops at a generate-local signal declaration in ``neorv32_prim.vhd``. Vector helpers
and further elaboration constructs also remain to be implemented.

The RTL path supports ``numeric_std.unsigned`` and ``numeric_std.signed`` addition, subtraction,
multiplication, equality/inequality and concatenation. Static
``to_unsigned(value, size)`` calls and conversions between ``unsigned``, ``signed``,
and ``std_logic_vector`` are supported. Signed arithmetic extends the sign bit.
``std_logic_signed`` vector addition, subtraction, multiplication and
equality/inequality are also supported.
Explicit binary signal initializers on locally clocked registers are preserved
as DFF ``INIT`` parameters.

Uninitialized arrays with one dynamically indexed, whole-word clocked write
site are inferred as canonical RAM primitives. Conditional write enables are
preserved, and clocked read assignments retain a separate output register,
including its enable and read-before-write behavior on address collisions.
Resetting address registers does not reset the RAM contents. Both ascending and
descending arrays with nonnegative 32-bit bounds are supported. Arrays with
initializers, multiple write sites, partial-word writes, writes inside loops,
writes in asynchronous-reset or generated processes, or direct connections to child-instance
ports retain the register/mux lowering.

Clock guards accept ``rising_edge(clk)`` or ``clk'event and clk = '1'``,
including parentheses around the guard or its operands, in both simple and
structured processes. Extra Boolean conditions on the clock guard remain
unsupported; place enables inside the edge guard.

Asynchronous reset accepts a scalar equality to ``'0'`` or ``'1'`` before
the edge guard: ``if rst = '0' then ... elsif rising_edge(clk) then ...``.
Both reset and clock must appear in the sensitivity list. Reset values must
resolve to binary constants per bit; mixed reset-to-zero and reset-to-one values
are supported. Bits omitted from the reset branch hold their state while reset
is active, including at clock edges. Enables and explicit register initializers
are preserved. Dynamic asynchronous loads, multiple reset sources, and assignments
outside the clock/reset guard are rejected. General event-driven processes,
nonbinary literals, multiple drivers, dynamic ``to_unsigned`` conversions and
general VHDL remain unsupported and are diagnosed. Argument and path errors
use the same Python exception categories as the other high-level loaders.
Frontend or lowering failures are reported as :class:`RuntimeError`.
RTL lowering errors include the original file, line, and column.

For separate RTL dependency files, use ``najaeda.naja.NLDB.loadVHDL`` on the
same database, loading dependencies before the top. A file with one entity that
requires generic values returns ``None`` when no ``top`` is specified; its
source is retained for later elaboration by its parent. An explicit ``top``
requires immediate elaboration and reports missing generic values. The raw API
also supports package-only files returning ``None``; the high-level loader
expects a completed top design.

Liberty and primitive libraries
-------------------------------

.. code-block:: python

   from najaeda import netlist

   netlist.load_liberty(["NangateOpenCellLibrary_typical.lib"])
   netlist.load_primitives("xilinx")

The primitives library is available through
:func:`najaeda.netlist.get_primitives_library` for scripts that need to inspect
or create primitive-backed models.

Built-in Python primitive libraries attach combinational/sequential timing
arcs and characterize clock, data, enable, reset/set, and memory terms.  When
authoring a custom primitive loader, use the raw timing-model API described in
:ref:`Primitive timing modeling <primitive-timing-modeling>`.

Naja interchange
----------------

.. code-block:: python

   from najaeda import netlist

   top = netlist.load_naja_if("design.naja")
   netlist.dump_naja_if("roundtrip.naja")

Naja interchange preserves the SNL database more directly than Verilog and is
useful for round-tripping internal analyses and transformations.

Verilog output
--------------

Use native dump methods when you need Verilog output from the underlying SNL
database.  The high-level package currently exposes Naja interchange directly;
for raw Verilog dump control, see :doc:`raw_api`.
