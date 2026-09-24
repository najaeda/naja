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
arrays, static indexing and slices, nested ``for`` loops and ``for generate``
statements, component instances, vector registers, synchronous reset/enable,
and immediate process variables. Generated instances can bind indexed or sliced
ports; integer constant tables can supply their generic values. Architecture
and package constants may use positional array aggregates, including
unconstrained array types with ``natural``, ``positive``, or ``integer`` indices.

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
or direct connections to child-instance ports retain the register/mux lowering.

Clock guards accept ``rising_edge(clk)`` or ``clk'event and clk = '1'``,
including parentheses around the guard or its operands, in both simple and
structured processes. Extra Boolean conditions on the clock guard remain
unsupported; place enables inside the edge guard. Asynchronous processes,
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
