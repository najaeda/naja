Expert Raw API
==============

``najaeda`` ships two Python API levels:

* :mod:`najaeda.netlist` is the supported high-level API for most tools.
* :mod:`najaeda.naja` is the raw compiled extension module, historically also
  importable as top-level ``naja`` in some build layouts.

The raw layer is useful, but it is intentionally not the first documentation
entry point.  It exposes native SNL objects directly and assumes that callers
understand database ownership, object lifetimes, uniquification, and the SNL
hierarchy model.

When to use the raw API
-----------------------

Use :mod:`najaeda.naja` when you need to:

* construct SNL objects using native factory methods;
* call a native method that is not wrapped by :mod:`najaeda.netlist`;
* inspect exact SNL IDs, paths, occurrences, libraries, or databases;
* use live SystemVerilog frontend intent helpers;
* debug the high-level API or compare wrapper behavior against native state.

Prefer :mod:`najaeda.netlist` for application code that edits hierarchical
designs.  If you edit raw objects directly, automatic high-level
uniquification does not protect you.

Importing the raw module
------------------------

Use the package export in user code:

.. code-block:: python

   from najaeda import naja

   print(naja.getVersion())
   universe = naja.NLUniverse.get()

Some development and test layouts also place the extension on ``PYTHONPATH``
as top-level ``naja``.  Prefer ``from najaeda import naja`` in documentation
and reusable scripts because it works with bundled package layouts too.  The
package-local extension takes precedence over a top-level module with the same
name, so an application file such as ``naja.py`` cannot shadow it.

Importing the native module does not write a performance report by default.
Set ``NAJA_PERF=1`` to write ``naja_perf.log``, or set ``NAJA_PERF`` to a file
path to select another destination. An unset or empty value disables the
performance report, which is appropriate when embedding Naja in a host Python
process.

Object identity and hashing
---------------------------

Raw SNL objects returned by repeated accessor calls are not guaranteed to be
the same Python wrapper object.  Do not use ``id(obj)`` to identify nets,
terms, instances, paths, occurrences, or other wrapped SNL objects across
calls.  Use ``==`` for equality and use the wrapped object itself as a
``dict`` key or ``set`` element when the object is hashable.  Objects with a
native ``NLID`` hash from that ``NLID``.  ``SNLPath`` and ``SNLOccurrence``
are hashable by composing the ``NLID`` values of their referenced objects.
``SNLAttribute`` is hashable from its name and typed value.  Other raw value
wrappers without an ``NLID`` are intentionally unhashable.

Relationship to high-level wrappers
-----------------------------------

High-level wrappers store or recover native objects internally:

.. list-table::
   :header-rows: 1
   :widths: 28 36 36

   * - High-level object
     - Typical raw object
     - Notes
   * - :class:`najaeda.netlist.Instance`
     - :class:`najaeda.naja.SNLInstance` plus :class:`najaeda.naja.SNLPath`
     - Carries occurrence context; edits may uniquify.
   * - :class:`najaeda.netlist.Term`
     - :class:`najaeda.naja.SNLTerm`, :class:`najaeda.naja.SNLBitTerm`, or
       :class:`najaeda.naja.SNLInstTerm`
     - May represent top terms or instance terms.
   * - :class:`najaeda.netlist.Net`
     - :class:`najaeda.naja.SNLNet`, :class:`najaeda.naja.SNLBitNet`, or
       concatenated bits
     - Use high-level wrappers for hierarchical net edits.
   * - :class:`najaeda.netlist.Equipotential`
     - :class:`najaeda.naja.SNLEquipotential`
     - Native object used for flat connectivity.
   * - :class:`najaeda.netlist.Attribute`
     - :class:`najaeda.naja.SNLAttribute`
     - Metadata attached to SNL design objects.

Raw construction example
------------------------

The exact factory signatures are native CPython bindings.  The following
pattern shows the intended ownership flow:

.. code-block:: python

   from najaeda import naja

   universe = naja.NLUniverse.create()
   db = naja.NLDB.create(universe)
   lib = naja.NLLibrary.create(db, "work")
   design = naja.SNLDesign.create(lib, "top")
   universe.setTopDesign(design)

   a = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "a")
   y = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "y")
   n = naja.SNLScalarNet.create(design, "n")

   a.setNet(n)
   y.setNet(n)

Object lifetime and safety
--------------------------

Raw objects are views of native C++ objects.  After calling ``destroy()`` or
after resetting/destroying the owning universe, database, library, or design,
do not keep using Python objects that referenced the destroyed native object.

The raw API also exposes shared model state.  Renaming, reconnecting, or
destroying an object through a raw handle mutates the underlying SNL object
directly.  If the same model is instantiated in multiple places, that change
can affect all occurrences unless you explicitly uniquify first.

SystemVerilog frontend intent helpers
-------------------------------------

The raw module includes expert helpers for live SystemVerilog frontend data:

* :func:`najaeda.naja.intent_available`
* :func:`najaeda.naja.live_compilation`
* :func:`najaeda.naja.ast_symbol_of`
* :func:`najaeda.naja.snl_objects_of`
* :func:`najaeda.naja.intent_parameters_of`
* :func:`najaeda.naja.intent_type_of`
* :func:`najaeda.naja.intent_package_member`

These helpers return plain Python data or capsules tied to the latest retained
SystemVerilog frontend state.  Treat capsules as opaque handles.

.. _primitive-timing-modeling:

Primitive timing modeling
--------------------------

Use the raw API when defining a primitive library in Python.  The high-level
:mod:`najaeda.netlist` API loads and consumes primitive libraries, while the
raw objects expose the definition-time timing decorators.

Primitive designs are hierarchy leaves.  Their interface, parameters, and
timing metadata can be defined directly, but they cannot contain instances.
Accordingly, ``SNLInstance.create(owner, model, name)`` raises
``RuntimeError`` when ``owner`` is primitive.  Passing a primitive as
``model`` remains the normal way to instantiate that primitive in a
non-primitive design.

Sequential primitives should declare both their timing arcs and the role of
each bit term:

.. code-block:: python

   from najaeda import naja

   dff = naja.SNLDesign.createPrimitive(primitives, "DFFRN")
   clk = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Input, "CLK")
   data = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Input, "D")
   reset_n = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Input, "RESET_N")
   output = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Output, "Q")

   naja.SNLDesign.addInputsToClockArcs([data, reset_n], clk)
   naja.SNLDesign.addClockToOutputsArcs(clk, output)
   clk.setRole(naja.SNLTermRole.Clock)
   data.setRole(naja.SNLTermRole.DataInput)
   reset_n.setRole(
       naja.SNLTermRole.AsyncReset, naja.SNLActiveLevel.Low)
   output.setRole(naja.SNLTermRole.DataOutput)

``SNLActiveLevel`` applies to asynchronous and synchronous reset/set roles.
The role and active level are inherited by instance terms.  Use
``getClockTerms()``, the other role-based design iterators, and the
``is_*`` term predicates to inspect the resulting model.

Parameterized combinational timing selects one design parameter and records
an arc set for each value.  Two-argument ``addCombinatorialArcs`` calls record
the default value's arcs; the three-argument form takes the parameter value
first:

.. code-block:: python

   naja.SNLParameter.create_string(cell, "MODE", "NORMAL")
   cell.setTimingModelParameter("MODE", "NORMAL")
   naja.SNLDesign.addCombinatorialArcs(input_a, output_a)
   naja.SNLDesign.addCombinatorialArcs(
       "CROSS", input_a, output_b)

The built-in loaders under :mod:`najaeda.primitives` use the same API, so
Python-defined primitives expose the same arc, role, and active-level
metadata as primitives constructed by the C++ DB0 and Liberty paths.

For primitives such as FPGA lookup tables whose function is held in an
instance parameter, use ``setTruthTableFromParameter(output, inputs,
parameter, bit_offset=0)``. The input list must follow design order and may
contain up to six terms. Naja resolves the instance override on first use and
caches the resulting truth table for that instance.

Liberty loading preserves every distinct sequential dependency declared by
setup, hold, or edge timing groups.  Consequently,
``getInputRelatedClocks()`` and ``getOutputRelatedClocks()`` can return more
than one clock for a term; repeated groups for the same term and clock are
deduplicated.  The generic timing-arc model does not retain Liberty ``when``
expressions, so conditionally enabled arcs are exposed conservatively as
unconditional dependencies.  The generic ``MemoryInterface`` remains a
single-clock abstraction: per-port clocks on a fully modeled multi-clock
memory are not represented by that interface.

Raw module reference
--------------------

The compiled extension exposes the following public objects in the current
binding.  Read this table as an expert index; the C++ SNL API remains the
semantic source of truth.

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Object
     - Main public methods and values
   * - :class:`najaeda.naja.NLUniverse`
     - ``create``, ``destroy``, ``get``, ``getDB``, ``getTopDB``, ``setTopDB``, ``getTopDesign``, ``setTopDesign``, ``getUserDBs``, ``getSNLDesign``, ``getObject``, ``applyDLE``, ``applyConstantPropagation``, ``getMaxFanout``, ``getMaxLogicLevel``
   * - :class:`najaeda.naja.NLDB`
     - ``create``, ``destroy``, ``getID``, ``getNLID``, ``isTopDB``, ``getLibraries``, ``getLibrary``, ``getGlobalLibraries``, ``getPrimitiveLibraries``, ``getTopDesign``, ``loadVerilog``, ``loadSystemVerilog``, ``loadVHDL``, ``loadLibertyPrimitives``, ``loadNajaIF``, ``dumpNajaIF``, ``dumpVerilog``
   * - :class:`najaeda.naja.NLLibrary`
     - ``create``, ``createPrimitives``, ``getDB``, ``getID``, ``getNLID``, ``getName``, ``setName``, ``isStandard``, ``isPrimitives``, ``getSNLDesign``, ``getSNLDesigns``, ``getLibrary``
   * - :class:`najaeda.naja.SNLDesign`
     - ``create``, ``createPrimitive``, ``clone``, ``destroy``, ``getName``, ``setName``, ``getDB``, ``getLibrary``, ``getID``, ``getNLID``, ``getRevisionCount``, ``getTerms``, ``getTerm``, ``getTermByID``, ``getScalarTerms``, ``getBusTerms``, ``getBundleTerms``, ``getNets``, ``getNet``, ``getScalarNets``, ``getBusNets``, ``getInstances``, ``getInstance``, ``getInstanceByID``, ``getInstanceByIDList``, ``getParameters``, ``getParameter``, ``addCombinatorialArcs``, ``addInputsToClockArcs``, ``addClockToOutputsArcs``, ``setTimingModelParameter``, ``getCombinatorialInputs``, ``getCombinatorialOutputs``, ``getClockRelatedInputs``, ``getClockRelatedOutputs``, ``getInputRelatedClocks``, ``getOutputRelatedClocks``, ``getClockTerms``, ``getAsyncResetTerms``, ``getAsyncSetTerms``, ``getSyncResetTerms``, ``getSyncSetTerms``, ``getDataInputTerms``, ``getOutputTerms``, ``setTruthTable``, ``setTruthTableFromParameter``, ``setTruthTables``, ``getTruthTable``, ``getTruthTableByOutputID``, ``isConst0``, ``isConst1``, ``isConst``, ``isBuf``, ``isInv``, ``isAnd``, ``isNand``, ``isOr``, ``isNor``, ``isXor``, ``isXnor``, ``isMux``, ``dumpVerilog``, ``dumpFullDotFile``, ``dumpContextDotFile``
   * - :class:`najaeda.naja.SNLInstance`
     - ``create``, ``destroy``, ``getName``, ``setName``, ``getID``, ``getNLID``, ``getDesign``, ``getModel``, ``getInstTerm``, ``getInstTerms``, ``getInstParameter``, ``getInstParameters``, ``getCombinatorialInputs``, ``getCombinatorialOutputs``, ``getClockRelatedInputs``, ``getClockRelatedOutputs``, ``getInputRelatedClocks``, ``getOutputRelatedClocks``
   * - :class:`najaeda.naja.SNLTerm` and term subclasses
     - ``Direction``, ``getName``, ``setName``, ``getDirection``, ``getDesign``, ``getNet``, ``setNet``, ``getBits``, ``getWidth``, ``getNLID``, ``getSourceLoc``, ``hasSourceLoc``; bit terms also expose ``setRole``, ``getRole``, ``getResetActiveLevel``, ``is_clock``, ``is_async_reset``, ``is_async_set``, ``is_sync_reset``, ``is_sync_set``, ``is_reset``, ``is_enable``, ``is_data``, ``is_data_input``, ``is_data_output``
   * - :class:`najaeda.naja.SNLNet` and net subclasses
     - ``Type``, ``getName``, ``setName``, ``getDesign``, ``getBits``, ``getWidth``, ``getType``, ``setType``, ``getTypeAsString``, ``isConstant``, ``isConstant0``, ``isConstant1``, ``getComponents``, ``getInstTerms``, ``getBitTerms``
   * - :class:`najaeda.naja.SNLInstTerm`
     - ``getInstance``, ``getBitTerm``, ``getNet``, ``setNet``, ``getDirection``, ``getRole``, ``getResetActiveLevel``, ``is_clock``, ``is_async_reset``, ``is_async_set``, ``is_sync_reset``, ``is_sync_set``, ``is_reset``, ``is_enable``, ``is_data``, ``is_data_input``, ``is_data_output``
   * - :class:`najaeda.naja.SNLTermRole`
     - ``Clock``, ``DataInput``, ``DataOutput``, ``AsyncReset``, ``AsyncSet``, ``SyncReset``, ``SyncSet``, ``Enable``, ``ScanInput``, ``ScanEnable``, ``MemoryReadAddress``, ``MemoryReadData``, ``MemoryWriteAddress``, ``MemoryWriteData``, ``MemoryWriteEnable``, ``Other``
   * - :class:`najaeda.naja.SNLActiveLevel`
     - ``High``, ``Low``, ``NA``
   * - :class:`najaeda.naja.SNLPath`
     - ``empty``, ``size``, ``getInstances``, ``getInstanceIDs``, ``getHeadInstance``, ``getTailInstance``, ``getHeadPath``, ``getTailPath``, ``getDesign``, ``getModel``
   * - :class:`najaeda.naja.SNLOccurrence`
     - ``isInstanceOccurrence``, ``getPath``, ``getInstance``, ``getInstTerm``, ``getNetComponent``, ``getDesign``
   * - :class:`najaeda.naja.SNLEquipotential`
     - ``Mode``, ``getTerms``, ``getInstTermOccurrences``, ``isConst0``, ``isConst1``, ``dumpDotFile``
   * - :class:`najaeda.naja.NLID`
     - ``from_string``, ``toTuple``, ``getType``, ``getDBID``, ``getLibraryID``, ``getDesignID``, ``getDesignObjectID``, ``getInstanceID``, ``getBit``, ``isDesign``, ``isInstance``, ``isNet``, ``isTerm``
   * - :class:`najaeda.naja.LogicCone`
     - ``FanIn``, ``FanOut``, ``getDirection``, ``getRoot``, ``getNodes``, ``getLeaves``, ``getNodeCount`` and snake_case aliases
   * - Module functions
     - :func:`najaeda.naja.getVersion`, :func:`najaeda.naja.getGitHash`,
       :func:`najaeda.naja.snapshot_manifest`,
       :func:`najaeda.naja.setLogLevel`, :func:`najaeda.naja.addLogFile`,
       :func:`najaeda.naja.clearLogSinks`,
       :func:`najaeda.naja.installLoggingHandler`, :func:`najaeda.naja.log`,
       :func:`najaeda.naja.logInfo`, :func:`najaeda.naja.logWarn`,
       :func:`najaeda.naja.logCritical` plus the SystemVerilog intent helpers
       listed above

Detailed raw reference
----------------------

When Sphinx can import the compiled extension, the following section exposes
clickable class and method anchors generated from the raw binding itself.
Documentation builders that do not have the binary available show the static
expert reference above.

.. only:: raw_naja_available

   .. py:currentmodule:: najaeda.naja

   Core database objects
   ~~~~~~~~~~~~~~~~~~~~~

   .. autosummary::

      NLUniverse
      NLDB
      NLLibrary
      SNLDesign

   .. autoclass:: najaeda.naja.NLUniverse
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.NLDB
      :members:
      :undoc-members:

   ``NLDB.loadSystemVerilog`` writes incremental frontend diagnostics to
   ``naja_sv_diagnostics.log`` by default. Pass ``diagnostics_report_path=None``
   to disable the report file and keep diagnostics console-only.

   ``NLDB.loadVHDL(file, top=None, diagnostics_report_path="naja_vhdl_diagnostics.log")`` loads one VHDL source file and returns its
   ``SNLDesign``. Load package files before their users in the same database:
   a package-only file returns ``None`` and preserves the current top design.
   A file containing one entity with required generic values also returns
   ``None`` when ``top`` is omitted: its source is retained until a parent
   supplies those values through a generic map. Pass ``top`` explicitly to
   require immediate elaboration and diagnose any missing generic values.
   RTL source files and package declarations are retained in the live design
   library for subsequent loads; they are not serialized in NajaIF. Load these
   dependencies through the raw API; the high-level loader expects a completed
   top design. RTL lowering errors include the original source file, line, and
   column, including errors in a dependency elaborated by a later load.
   Package function declarations and bodies are also retained for later loads.
   Pure integer/boolean helpers can elaborate constants and bounds with static
   arguments; overload resolution and runtime/vector function evaluation are
   not yet supported. A package-only load validates parsing, not every function
   body's evaluability. Unsupported calls are diagnosed when elaboration needs
   them. The high-level loader uses the same evaluator once a top is available.
   Supported record ports become single ``SNLBusTerm`` objects with descending
   indices from total width minus one to zero. Fields occupy consecutive bits in
   declaration order, recursively; record field metadata is not exposed as raw
   SNL objects. Use :doc:`loading` for the supported record and unresolved-logic
   subset and current NEORV32 limitations.
   The RTL path infers a unique uninstantiated root entity, including references
   inside nested generate statements. Otherwise, specify ``top`` explicitly.
   The basic structural hierarchy path still requires an explicit ``top``.

   Architecture-local pure functions also support static integer/boolean evaluation.
   Bodies and default arguments use declarations visible at the function declaration,
   including enclosing generics and scalar constants. Runtime/vector evaluation,
   overloading, and architecture function forward declarations remain unsupported.
   Static function loops support unlabeled ``exit`` and ``exit when``; labeled
   exits and process-loop exits remain unsupported.

   The loader excludes simulation regions marked by ``pragma``, ``synthesis``,
   or ``synopsys`` ``translate_off/on`` comments, preserving source locations.
   Assertion and report statements are ignored, including conditions and
   ``severity failure``; configuration assertions do not validate generics.
   Each load logs the first occurrence of each warning code to the console.
   The diagnostics report records all occurrences with source path, line, and
   column, and is overwritten per load. Set ``diagnostics_report_path`` to a
   string path to choose the report, or ``None`` for console-only output.
   Warning codes currently include ``ignored-assertion`` and ``ignored-report``.
   Retained dependency sources are not reported again during internal re-parsing.
   The high-level ``netlist.load_vhdl`` exposes the same option and also accepts
   ``os.PathLike`` paths.
   Nested or unbalanced exclusion directives are errors.

   Concurrent ``with ... select`` assignments support static/grouped choices
   and ``others``, with the same coverage checks as case statements.

   Input port maps accept binary character, string, and binary/octal/hex
   bit-string literals, checked against the formal port type and width.
   Output literals and general expression/aggregate actuals remain unsupported.

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

   A single dynamically indexed whole-word clocked write site can infer an
   uninitialized array as an NLDB0 RAM primitive. Read registers remain explicit
   DFFs, preserving enables and old-data read/write collisions. Address reset
   leaves memory contents intact. See :doc:`loading` for inference limits and
   cases that retain register/mux lowering.

   The experimental two-state RTL subset supports constrained arrays, package
   array types and positional constant aggregates, binary/octal/hex literals,
   static slices and loops, nested ``for generate`` and static
   ``if``/``elsif``/``else generate`` statements containing assignments, clocked
   processes, and component/direct-entity instances. Generate-local signals,
   constants, arrays, records, and enumerations have separate bindings and nets per elaborated
   body; local arrays use register/mux lowering. Internal enum signals, record
   fields, and arrays preserve nominal type checking and explicit initialization;
   enum-valued ports are not yet supported. The subset also supports
   nested sequential cases with static choices, ranges, and ``others``, plus
   combinational processes with complete assignment coverage and explicit or
   ``all`` sensitivity. Explicit sensitivity entries accept nested record fields
   and validate coverage per signal bit. The subset includes integer-indexed
   ROM/RAM reads and clocked writes, and multiple clocked
   processes with synchronous reset/enable and constant asynchronous reset/set.
   Asynchronous state uses canonical NLDB0 DFFRN, DFFR, or DFFS primitives;
   bits omitted from the reset branch hold while reset is active. See
   :doc:`loading` for the supported reset idiom and restrictions.
   Both simple and structured processes
   accept ``rising_edge(clk)`` and ``clk'event and clk = '1'`` guards, including
   parentheses around the guard or its event/level operands. The event and level
   names must match the same scalar input clock in the sensitivity list; falling
   edges and extra Boolean conditions on the guard are rejected.
   Nonnegative constrained integer
   counters, ``std_logic_unsigned`` addition/subtraction and
   ``conv_integer(std_logic_vector)`` (with ``std_logic_arith`` imported) are
   supported. Component binding requires a matching visible declaration and
   supports named or positional ports (including static indices and slices)
   and integer generic specialization. A generic can omit its default when its
   value is supplied by the instance. Architecture/package integer constants
   and one-dimensional integer constant tables are evaluated statically.
   Positional aggregates can constrain an otherwise unconstrained array type;
   inferred indices begin at the index subtype's left bound (0 for ``natural``,
   1 for ``positive``, and -2147483648 for the supported 32-bit ``integer``).
   Explicit bounds preserve their direction and are checked against the subtype.
   Hardware operates on legal subtype/index values; simulation bounds checks
   and nine-valued initialization are not implemented. Nonbinary literals,
   conflicting drivers, unsupported package bodies, and unsupported operations
   are rejected. ``numeric_std.unsigned`` and ``numeric_std.signed`` vector addition, subtraction,
   multiplication, equality/inequality and concatenation are supported, including
   mixed operand widths for arithmetic and comparisons. Multiplication produces
   the sum of operand widths; addition/subtraction produce their maximum width.
   Explicit binary signal initializers on locally clocked registers become DFF
   ``INIT`` parameters. Initialization on other drivers is rejected.
   Static ``numeric_std.to_unsigned(value, size)`` accepts natural values through
   2147483647 and sizes from 1 through 65536; values wider than the result retain
   their low bits, as specified by the conversion. Dynamic conversions and null
   vectors are rejected. Conversions between ``unsigned``, ``signed``, and
   ``std_logic_vector`` preserve bit positions. Signed arithmetic sign-extends
   operands; mixed signed/unsigned operands require explicit conversions.
   With ``std_logic_signed`` visible, vector addition, subtraction, multiplication
   and equality/inequality use signed operands;
   ambiguous numeric operations importing both signed and unsigned overloads
   are rejected.
   Component declarations can also appear in the architecture declarative part,
   and port declarations may explicitly specify the ``signal`` class.

   These features are also available through the high-level
   ``najaeda.netlist.load_vhdl`` loader.

   The raw ``NLDB`` Verilog, SystemVerilog, VHDL, and Liberty loaders report malformed
   Python arguments with standard :class:`TypeError` or :class:`ValueError`
   exceptions.  List-entry errors include the option name and index, such as
   ``files[1]`` or ``defines[0]``.  Native parser and elaboration failures
   remain ``RuntimeError`` subclasses; SystemVerilog failures use the more
   specific ``SystemVerilog*`` exception classes and expose structured
   diagnostic details where available.

   .. autoclass:: najaeda.naja.NLLibrary
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLDesign
      :members:
      :undoc-members:

   Design objects
   ~~~~~~~~~~~~~~

   .. autosummary::

      SNLInstance
      SNLTerm
      SNLScalarTerm
      SNLBusTerm
      SNLBundleTerm
      SNLBitTerm
      SNLBusTermBit
      SNLNet
      SNLScalarNet
      SNLBusNet
      SNLBitNet
      SNLInstTerm
      SNLInstParameter
      SNLParameter
      SNLAttribute

   .. autoclass:: najaeda.naja.SNLInstance
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLTerm
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLScalarTerm
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLBusTerm
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLBundleTerm
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLBitTerm
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLBusTermBit
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLNet
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLScalarNet
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLBusNet
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLBitNet
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLInstTerm
      :members:
      :undoc-members:
      :inherited-members:

   .. autoclass:: najaeda.naja.SNLInstParameter
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLParameter
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLAttribute
      :members:
      :undoc-members:

   Hierarchy and connectivity
   ~~~~~~~~~~~~~~~~~~~~~~~~~~

   .. autosummary::

      SNLPath
      SNLOccurrence
      SNLEquipotential
      SNLNetComponent
      NLID
      LogicCone

   .. autoclass:: najaeda.naja.SNLPath
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLOccurrence
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLEquipotential
      :members:
      :undoc-members:

      Pass ``mode=SNLEquipotential.Mode.TraverseAssigns`` to the constructor
      to cross Assign instances and omit their instance terminals.  The
      default is ``SNLEquipotential.Mode.Standard``.

   .. autoclass:: najaeda.naja.SNLNetComponent
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.NLID
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.LogicCone
      :members:
      :undoc-members:

   Enum-like values
   ~~~~~~~~~~~~~~~~

   .. autosummary::

      SNLTerm.Direction
      SNLNet.Type
      SNLEquipotential.Mode
      SNLTermRole
      SNLActiveLevel

   .. autoclass:: najaeda.naja.SNLTerm.Direction
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLNet.Type
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLEquipotential.Mode
      :members:
      :undoc-members:

   ``SNLNet.Type.AssignX`` and ``SNLNet.Type.AssignZ`` represent structural
   four-state constants.  Use ``SNLNet.isConstantX()`` and
   ``SNLNet.isConstantZ()`` to distinguish them; ``isConstant()`` accepts all
   four assign values while the zero and one predicates remain binary-only.

   .. autoclass:: najaeda.naja.SNLTermRole
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLActiveLevel
      :members:
      :undoc-members:

   Module functions
   ~~~~~~~~~~~~~~~~

   .. autosummary::

      getVersion
      getGitHash
      snapshot_manifest
      setLogLevel
      addLogFile
      clearLogSinks
      installLoggingHandler
      log
      logInfo
      logWarn
      logCritical
      intent_available
      live_compilation
      ast_symbol_of
      snl_objects_of
      intent_parameters_of
      intent_type_of
      intent_package_member

   .. autofunction:: najaeda.naja.getVersion
   .. autofunction:: najaeda.naja.getGitHash

   ``snapshot_manifest(path)`` reads only a snapshot's ``snl.mf`` manifest;
   it does not load the Cap'n Proto payload or create an ``NLUniverse``.  It
   returns the schema version and the Naja producer version / Git hash.  At
   present, loading a snapshot requires an exact match of both producer
   values with the reader build; regenerate snapshots after changing builds.

   .. autofunction:: najaeda.naja.snapshot_manifest
   .. autofunction:: najaeda.naja.setLogLevel
   .. autofunction:: najaeda.naja.addLogFile
   .. autofunction:: najaeda.naja.clearLogSinks
   .. autofunction:: najaeda.naja.installLoggingHandler
   .. autofunction:: najaeda.naja.log
   .. autofunction:: najaeda.naja.logInfo
   .. autofunction:: najaeda.naja.logWarn
   .. autofunction:: najaeda.naja.logCritical
   .. autofunction:: najaeda.naja.intent_available
   .. autofunction:: najaeda.naja.live_compilation
   .. autofunction:: najaeda.naja.ast_symbol_of
   .. autofunction:: najaeda.naja.snl_objects_of
   .. autofunction:: najaeda.naja.intent_parameters_of
   .. autofunction:: najaeda.naja.intent_type_of
   .. autofunction:: najaeda.naja.intent_package_member
