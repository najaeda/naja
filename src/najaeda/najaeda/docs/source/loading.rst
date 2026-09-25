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

Python VHDL loading emits a :class:`RuntimeWarning` on each parser invocation
because the frontend is in Beta mode. Its supported language subset and behavior
may change between releases.

The RTL path can infer the top when exactly one entity is not instantiated by
another design unit, including instances inside nested generate statements.
If there are several roots or no root, pass ``top`` explicitly.

VHDL loading is experimental and implements a bounded, two-state RTL subset.
It supports integer and boolean generic defaults and explicit generic maps, constrained
arrays, static indexing and slices, nested ``for`` loops, ``for generate`` and
static ``if``/``elsif``/``else generate`` statements, component instances, vector registers, synchronous reset/enable,
asynchronous reset/set,
and immediate process variables. Generated instances can bind indexed or sliced
ports; integer constant tables can supply their generic values. Architecture
and package constants may use positional array aggregates, including
unconstrained array types with ``natural``, ``positive``, or ``integer`` indices.

Conditional generates select the first true branch at elaboration time, using
static boolean expressions built from integer/boolean generics, constants, enclosing
loop parameters, and supported static package functions. Only the selected
branch creates hardware. Generate bodies may contain assignments, instances,
clocked processes, and nested conditional or loop generates. Instance names
retain generate scope prefixes. Generate-local signals, constants, array types,
and record types are elaborated in declaration order. Each loop iteration has
independent local nets and register initialization; local names do not escape
their body. Local arrays retain register/mux lowering. Shadowing an outer name,
local subtypes/functions/components, alternative labels, and explicit branch-body
``end`` statements remain unsupported and are diagnosed.

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

Identifier-based enumerated types declared in packages, architectures, and
generate bodies support internal signals, variables, constants, record fields,
and arrays. Literals are encoded by declaration position using at least one bit;
explicit initializers are preserved. Type identity is retained for assignment and
equality/inequality checking. Cases may cover all declared literals without an
``others`` branch, including types whose literal count is not a power of two.
Character literals, overloaded literal names, enum ports, subtype constraints,
ordering comparisons, enum attributes, and enum-valued static functions remain
unsupported. Implicit enum initialization is not synthesized.

Pure package functions with static integer and boolean arguments can compute
constants, generic defaults, vector bounds, and constant expressions. The
supported bodies contain local scalar variables/constants, variable assignments,
``if``/``elsif``/``else``, bounded ``for`` loops, nested function calls, and early
``return`` statements. Unlabeled ``exit`` and ``exit when`` leave the innermost
function loop; labeled exits and process-loop exits remain unsupported. Positional and named actuals and default arguments are
supported. Defaults and function bodies resolve package declarations rather
than caller-local names. Integer arithmetic includes division, ``mod``, ``rem``,
exponentiation, and ``abs``; boolean comparisons and logical operators preserve
short-circuit evaluation. Subtype violations, overflow, missing returns,
ambiguous overloads, and excessive recursion or evaluation work are diagnosed.
Function calls with runtime arguments, vector/string function evaluation, and
impure functions remain unsupported. Array attributes and interface defaults
are retained by the parser, but their elaboration is not yet supported.

Architecture-local pure functions also support static integer/boolean evaluation.
Bodies and default arguments use declarations visible at the function declaration,
including enclosing generics and scalar constants. Runtime/vector evaluation,
overloading, and architecture function forward declarations remain unsupported.

Input port maps accept binary character, string, and binary/octal/hex bit-string
literals in positional or named associations, for direct entities and components.
The formal port supplies the type and bit order. Width/type mismatches, non-binary
values, and literals bound to output ports are rejected. General expression and
aggregate port actuals remain unsupported.

Named port associations may select vector elements, slices, and record fields
on the formal side, for example ``data_i(0) => data_bit``. Individual associations
must be consecutive and cover every scalar subelement exactly once. Overlaps,
missing elements, invalid directions or bounds, and type/width mismatches are
errors. Formal index expressions currently support integer literals, locally
static integer constants, and predefined arithmetic; generics and generate
iterators cannot be used as formal indices. Actual selections may still use
elaboration-time generic or generate values. Component selections use the
component's bounds and bind to the entity by position. Whole output ports may be associated with ``open``, including scalar,
vector, and record ports in positional or named maps. Their instance terminals
remain unconnected, matching SV empty output connections, without a warning.
Input defaults are not yet supported, so open inputs remain rejected. Individual
formal elements or slices cannot be associated with ``open``.
As with connected record ports, their type package currently needs to be visible
in the instantiating scope as well as the child entity.

Processes may repeat their opening label in ``end process label;``, including
combinational, clocked, and asynchronous-reset processes inside generates.
The closing label is optional; when present it must match the opening label.
Basic identifiers are case-insensitive, while extended identifiers retain case.

Arrays may use an enumeration as their index type, for example
``type requests_t is array(device_t) of request_t;``. Element order follows the
enumeration declaration, with nominal index-type checks. Enum literals and
constants select elements directly, including record elements in port maps.
Explicit enum ranges, descending slices, and unconstrained enum-indexed arrays
with explicit object bounds are supported. Runtime enum indices use mux reads
and decoded signal writes; clocked enum-indexed arrays retain register lowering
rather than RAM inference. Integer-indexed arrays reject enum indices, and
enum-indexed arrays reject integer or unrelated-enum indices. Enumeration-indexed
integer constant tables and enumeration subtype declarations remain unsupported.


Concurrent selected assignments (``with ... select``) support static choices,
grouped choices, and a final ``others`` alternative, including inside generates.
They share case-statement type, duplicate-choice, and coverage checks and lower
to combinational multiplexers. Delays and matching selections remain unsupported.

The netlist loader honors case-insensitive ``translate_off`` / ``translate_on``
comment directives prefixed by ``pragma``, ``synthesis``, or ``synopsys``.
Excluded simulation code contributes no hardware; source locations are preserved.
Nested, unmatched, and unterminated regions are rejected. Ordinary lexer/parser
calls retain simulation code unless their synthesis option is enabled. Unmarked
file declarations and other simulation operations remain unsupported.
In synthesis mode, assertion and report statements are parsed but ignored,
including their conditions, messages, and severity (even ``failure``).
Configuration assertions therefore do not validate generic values for now.
Each load emits one console warning per code (``ignored-assertion`` or
``ignored-report``). All occurrences, including source locations, are written to
``naja_vhdl_diagnostics.log``, overwritten per load. Internal re-parsing of
retained dependencies does not repeat their warnings. Unsupported hardware
constructs still fail loading. Choose a report path or select console-only output:

.. code-block:: python

   top = netlist.load_vhdl("design.vhd", diagnostics_report_path="logs/vhdl.log")
   # Alternatively: diagnostics_report_path=None for console-only warnings.


The NEORV32 default ``neorv32_top`` configuration remains a development target.
The complete core file list parses and resolves its named-library imports when
loaded into ``library="neorv32"``. Boolean generics now elaborate; the next
unsupported generic is ``BOOT_ADDR_CUSTOM : std_ulogic_vector(31 downto 0)``
at ``neorv32_top.vhd:30`` in the development checkout. Vector generics, runtime
vector helpers, and further elaboration constructs remain unsupported; the
complete top has not been validated.

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

Combinational processes accept ``process(all)`` or an explicit sensitivity list
covering every signal bit read. Explicit entries may name whole records or nested
record fields, such as ``process(packet.inner.data, packet.inner.flag)``. A parent
record covers its descendants; listing one field does not cover its siblings.
Indexed or sliced sensitivity entries remain unsupported. The same coverage check
applies to assignments outside a clock guard. Their bodies support ordered signal assignments,
immediate variables, static loops, and nested ``if``/``elsif``/``else`` and ``case`` branches.
Default assignments followed by partial conditional overrides preserve the last
assignment to each bit. Every driven bit must be assigned on every path; latch
inference is rejected. Variables must be assigned before use. Reading a signal
written by the same process, variable declaration initializers, and ``wait``
statements remain unsupported. Signal reads retain VHDL's scheduled
assignment semantics and are not replaced by earlier assignments in the body.

Sequential ``case`` statements support binary scalar/vector, boolean, and
nonnegative integer or enumerated selectors, grouped static choices (``|``), integer ranges,
and a final ``others`` alternative. ``null`` preserves the incoming assignment
state. Duplicate or overlapping choices and runtime choices are rejected.
Without ``others``, binary alternatives must cover the entire selector domain;
integer selectors require ``others`` in this profile. Choice expansion is bounded
to 4096 values. Cases in static package-function bodies remain unsupported.

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

Boolean generic defaults and actuals use typed static expressions, including
references to earlier generics and supported pure scalar functions. Positional
and named maps work for both direct entity instances and component bindings.
Boolean values remain distinct from integers: ``0``/``1`` cannot replace
``false``/``true``. Boolean specializations have separate cached models, and
conditions can select generate branches or drive supported boolean expressions.
Required boolean generics follow the same deferred dependency-loading convention
as required integer generics. Vector generic values are not yet supported.

HDL destination libraries
-------------------------

Both loaders accept a keyword-only ``library`` name, defaulting to ``"DESIGN"``:

.. code-block:: python

   top = netlist.load_vhdl("top.vhd", library="neorv32")
   top = netlist.load_system_verilog("top.sv", library="implementation")

The destination is a root ``NLLibrary`` in the current database. Basic names
match case-insensitively; extended names retain their case. Missing destinations
are created, while multiple matching roots are an error. Nested libraries and
other databases are not searched. This option chooses where SV designs are
stored; it does not add cross-library SV source binding.

For VHDL, named references such as ``cells.leaf`` and ``cells.types.all`` resolve
against these roots after a corresponding ``library cells;`` clause. ``work``
means the defining library of the current entity or package. Dependencies retain
their own library context, and identically named units in different libraries
remain distinct. ``std.standard`` and supported IEEE packages use built-in
providers; general standard-library source compilation is not supported.

Load package-only or generic-dependent files through the raw API first:

.. code-block:: python

   db.loadVHDL("types.vhd", library="cells")
   db.loadVHDL("leaf.vhd", library="cells")
   top_design = db.loadVHDL("top.vhd", top="top", library="DESIGN")

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
