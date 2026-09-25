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
   * - :class:`najaeda.naja.SNLParameter`
     - ``create_decimal``, ``create_binary``, ``create_boolean``, ``create_string`` (optional default value), ``hasDefaultValue``, ``getValue``, ``getName``, ``getDesign``, ``destroy``
   * - :class:`najaeda.naja.NLUniverse`
     - :meth:`create <najaeda.naja.NLUniverse.create>`, :meth:`destroy <najaeda.naja.NLUniverse.destroy>`, :meth:`get <najaeda.naja.NLUniverse.get>`, :meth:`getDB <najaeda.naja.NLUniverse.getDB>`, :meth:`getTopDB <najaeda.naja.NLUniverse.getTopDB>`, :meth:`setTopDB <najaeda.naja.NLUniverse.setTopDB>`, :meth:`getTopDesign <najaeda.naja.NLUniverse.getTopDesign>`, :meth:`setTopDesign <najaeda.naja.NLUniverse.setTopDesign>`, :meth:`getUserDBs <najaeda.naja.NLUniverse.getUserDBs>`, :meth:`getSNLDesign <najaeda.naja.NLUniverse.getSNLDesign>`, :meth:`getObject <najaeda.naja.NLUniverse.getObject>`, :meth:`applyDLE <najaeda.naja.NLUniverse.applyDLE>`, :meth:`applyConstantPropagation <najaeda.naja.NLUniverse.applyConstantPropagation>`, :meth:`getMaxFanout <najaeda.naja.NLUniverse.getMaxFanout>`, :meth:`getMaxLogicLevel <najaeda.naja.NLUniverse.getMaxLogicLevel>`
   * - :class:`najaeda.naja.NLDB`
     - :meth:`create <najaeda.naja.NLDB.create>`, :meth:`destroy <najaeda.naja.NLDB.destroy>`, :meth:`getID <najaeda.naja.NLDB.getID>`, :meth:`getNLID <najaeda.naja.NLDB.getNLID>`, :meth:`isTopDB <najaeda.naja.NLDB.isTopDB>`, :meth:`getLibraries <najaeda.naja.NLDB.getLibraries>`, :meth:`getLibrary <najaeda.naja.NLDB.getLibrary>`, :meth:`getGlobalLibraries <najaeda.naja.NLDB.getGlobalLibraries>`, :meth:`getPrimitiveLibraries <najaeda.naja.NLDB.getPrimitiveLibraries>`, :meth:`getTopDesign <najaeda.naja.NLDB.getTopDesign>`, :meth:`loadVerilog <najaeda.naja.NLDB.loadVerilog>`, :meth:`loadSystemVerilog <najaeda.naja.NLDB.loadSystemVerilog>`, :meth:`loadVHDL <najaeda.naja.NLDB.loadVHDL>`, :meth:`loadLibertyPrimitives <najaeda.naja.NLDB.loadLibertyPrimitives>`, :meth:`loadNajaIF <najaeda.naja.NLDB.loadNajaIF>`, :meth:`dumpNajaIF <najaeda.naja.NLDB.dumpNajaIF>`, :meth:`dumpVerilog <najaeda.naja.NLDB.dumpVerilog>`
   * - :class:`najaeda.naja.NLLibrary`
     - :meth:`create <najaeda.naja.NLLibrary.create>`, :meth:`createPrimitives <najaeda.naja.NLLibrary.createPrimitives>`, :meth:`getDB <najaeda.naja.NLLibrary.getDB>`, :meth:`getID <najaeda.naja.NLLibrary.getID>`, :meth:`getNLID <najaeda.naja.NLLibrary.getNLID>`, :meth:`getName <najaeda.naja.NLLibrary.getName>`, :meth:`setName <najaeda.naja.NLLibrary.setName>`, :meth:`isStandard <najaeda.naja.NLLibrary.isStandard>`, :meth:`isPrimitives <najaeda.naja.NLLibrary.isPrimitives>`, :meth:`getSNLDesign <najaeda.naja.NLLibrary.getSNLDesign>`, :meth:`getSNLDesigns <najaeda.naja.NLLibrary.getSNLDesigns>`, :meth:`getLibrary <najaeda.naja.NLLibrary.getLibrary>`, :meth:`getLibraries <najaeda.naja.NLLibrary.getLibraries>`
   * - :class:`najaeda.naja.SNLDesign`
     - :meth:`create <najaeda.naja.SNLDesign.create>`, :meth:`createPrimitive <najaeda.naja.SNLDesign.createPrimitive>`, :meth:`clone <najaeda.naja.SNLDesign.clone>`, :meth:`destroy <najaeda.naja.SNLDesign.destroy>`, :meth:`getName <najaeda.naja.SNLDesign.getName>`, :meth:`setName <najaeda.naja.SNLDesign.setName>`, :meth:`getDB <najaeda.naja.SNLDesign.getDB>`, :meth:`getLibrary <najaeda.naja.SNLDesign.getLibrary>`, :meth:`getID <najaeda.naja.SNLDesign.getID>`, :meth:`getNLID <najaeda.naja.SNLDesign.getNLID>`, :meth:`getRevisionCount <najaeda.naja.SNLDesign.getRevisionCount>`, :meth:`getTerms <najaeda.naja.SNLDesign.getTerms>`, :meth:`getTerm <najaeda.naja.SNLDesign.getTerm>`, :meth:`getTermByID <najaeda.naja.SNLDesign.getTermByID>`, :meth:`getScalarTerms <najaeda.naja.SNLDesign.getScalarTerms>`, :meth:`getBusTerms <najaeda.naja.SNLDesign.getBusTerms>`, :meth:`getBundleTerms <najaeda.naja.SNLDesign.getBundleTerms>`, :meth:`getNets <najaeda.naja.SNLDesign.getNets>`, :meth:`getNet <najaeda.naja.SNLDesign.getNet>`, :meth:`getScalarNets <najaeda.naja.SNLDesign.getScalarNets>`, :meth:`getBusNets <najaeda.naja.SNLDesign.getBusNets>`, :meth:`getInstances <najaeda.naja.SNLDesign.getInstances>`, :meth:`getInstance <najaeda.naja.SNLDesign.getInstance>`, :meth:`getInstanceByID <najaeda.naja.SNLDesign.getInstanceByID>`, :meth:`getInstanceByIDList <najaeda.naja.SNLDesign.getInstanceByIDList>`, :meth:`getParameters <najaeda.naja.SNLDesign.getParameters>`, :meth:`getParameter <najaeda.naja.SNLDesign.getParameter>`, :meth:`addCombinatorialArcs <najaeda.naja.SNLDesign.addCombinatorialArcs>`, :meth:`addInputsToClockArcs <najaeda.naja.SNLDesign.addInputsToClockArcs>`, :meth:`addClockToOutputsArcs <najaeda.naja.SNLDesign.addClockToOutputsArcs>`, :meth:`setTimingModelParameter <najaeda.naja.SNLDesign.setTimingModelParameter>`, :meth:`getCombinatorialInputs <najaeda.naja.SNLDesign.getCombinatorialInputs>`, :meth:`getCombinatorialOutputs <najaeda.naja.SNLDesign.getCombinatorialOutputs>`, :meth:`getClockRelatedInputs <najaeda.naja.SNLDesign.getClockRelatedInputs>`, :meth:`getClockRelatedOutputs <najaeda.naja.SNLDesign.getClockRelatedOutputs>`, :meth:`getInputRelatedClocks <najaeda.naja.SNLDesign.getInputRelatedClocks>`, :meth:`getOutputRelatedClocks <najaeda.naja.SNLDesign.getOutputRelatedClocks>`, :meth:`getClockTerms <najaeda.naja.SNLDesign.getClockTerms>`, :meth:`getAsyncResetTerms <najaeda.naja.SNLDesign.getAsyncResetTerms>`, :meth:`getAsyncSetTerms <najaeda.naja.SNLDesign.getAsyncSetTerms>`, :meth:`getSyncResetTerms <najaeda.naja.SNLDesign.getSyncResetTerms>`, :meth:`getSyncSetTerms <najaeda.naja.SNLDesign.getSyncSetTerms>`, :meth:`getDataInputTerms <najaeda.naja.SNLDesign.getDataInputTerms>`, :meth:`getOutputTerms <najaeda.naja.SNLDesign.getOutputTerms>`, :meth:`setTruthTable <najaeda.naja.SNLDesign.setTruthTable>`, :meth:`setTruthTableFromParameter <najaeda.naja.SNLDesign.setTruthTableFromParameter>`, :meth:`setTruthTables <najaeda.naja.SNLDesign.setTruthTables>`, :meth:`getTruthTable <najaeda.naja.SNLDesign.getTruthTable>`, :meth:`getTruthTableByOutputID <najaeda.naja.SNLDesign.getTruthTableByOutputID>`, :meth:`isConst0 <najaeda.naja.SNLDesign.isConst0>`, :meth:`isConst1 <najaeda.naja.SNLDesign.isConst1>`, :meth:`isConst <najaeda.naja.SNLDesign.isConst>`, :meth:`isBuf <najaeda.naja.SNLDesign.isBuf>`, :meth:`isInv <najaeda.naja.SNLDesign.isInv>`, :meth:`isAnd <najaeda.naja.SNLDesign.isAnd>`, :meth:`isNand <najaeda.naja.SNLDesign.isNand>`, :meth:`isOr <najaeda.naja.SNLDesign.isOr>`, :meth:`isNor <najaeda.naja.SNLDesign.isNor>`, :meth:`isXor <najaeda.naja.SNLDesign.isXor>`, :meth:`isXnor <najaeda.naja.SNLDesign.isXnor>`, :meth:`isMux <najaeda.naja.SNLDesign.isMux>`, :meth:`dumpVerilog <najaeda.naja.SNLDesign.dumpVerilog>`, :meth:`dumpFullDotFile <najaeda.naja.SNLDesign.dumpFullDotFile>`, :meth:`dumpContextDotFile <najaeda.naja.SNLDesign.dumpContextDotFile>`
   * - :class:`najaeda.naja.SNLInstance`
     - :meth:`create <najaeda.naja.SNLInstance.create>`, :meth:`destroy <najaeda.naja.SNLInstance.destroy>`, :meth:`getName <najaeda.naja.SNLInstance.getName>`, :meth:`setName <najaeda.naja.SNLInstance.setName>`, :meth:`getID <najaeda.naja.SNLInstance.getID>`, :meth:`getNLID <najaeda.naja.SNLInstance.getNLID>`, :meth:`getDesign <najaeda.naja.SNLInstance.getDesign>`, :meth:`getModel <najaeda.naja.SNLInstance.getModel>`, :meth:`getInstTerm <najaeda.naja.SNLInstance.getInstTerm>`, :meth:`getInstTerms <najaeda.naja.SNLInstance.getInstTerms>`, :meth:`getInstParameter <najaeda.naja.SNLInstance.getInstParameter>`, :meth:`getInstParameters <najaeda.naja.SNLInstance.getInstParameters>`, :meth:`getCombinatorialInputs <najaeda.naja.SNLInstance.getCombinatorialInputs>`, :meth:`getCombinatorialOutputs <najaeda.naja.SNLInstance.getCombinatorialOutputs>`, :meth:`getClockRelatedInputs <najaeda.naja.SNLInstance.getClockRelatedInputs>`, :meth:`getClockRelatedOutputs <najaeda.naja.SNLInstance.getClockRelatedOutputs>`, :meth:`getInputRelatedClocks <najaeda.naja.SNLInstance.getInputRelatedClocks>`, :meth:`getOutputRelatedClocks <najaeda.naja.SNLInstance.getOutputRelatedClocks>`
   * - :class:`najaeda.naja.SNLTerm` and term subclasses
     - :class:`Direction <najaeda.naja.SNLTerm.Direction>`, :meth:`getName <najaeda.naja.SNLTerm.getName>`, :meth:`setName <najaeda.naja.SNLTerm.setName>`, :meth:`getDirection <najaeda.naja.SNLTerm.getDirection>`, :meth:`getDesign <najaeda.naja.SNLTerm.getDesign>`, :meth:`getNet <najaeda.naja.SNLTerm.getNet>`, :meth:`setNet <najaeda.naja.SNLTerm.setNet>`, :meth:`getBits <najaeda.naja.SNLTerm.getBits>`, :meth:`getWidth <najaeda.naja.SNLTerm.getWidth>`, :meth:`getNLID <najaeda.naja.SNLTerm.getNLID>`, :meth:`getSourceLoc <najaeda.naja.SNLTerm.getSourceLoc>`, :meth:`hasSourceLoc <najaeda.naja.SNLTerm.hasSourceLoc>`; bit terms also expose :meth:`setRole <najaeda.naja.SNLBitTerm.setRole>`, :meth:`getRole <najaeda.naja.SNLBitTerm.getRole>`, :meth:`getResetActiveLevel <najaeda.naja.SNLBitTerm.getResetActiveLevel>`, :meth:`is_clock <najaeda.naja.SNLBitTerm.is_clock>`, :meth:`is_async_reset <najaeda.naja.SNLBitTerm.is_async_reset>`, :meth:`is_async_set <najaeda.naja.SNLBitTerm.is_async_set>`, :meth:`is_sync_reset <najaeda.naja.SNLBitTerm.is_sync_reset>`, :meth:`is_sync_set <najaeda.naja.SNLBitTerm.is_sync_set>`, :meth:`is_reset <najaeda.naja.SNLBitTerm.is_reset>`, :meth:`is_enable <najaeda.naja.SNLBitTerm.is_enable>`, :meth:`is_data <najaeda.naja.SNLBitTerm.is_data>`, :meth:`is_data_input <najaeda.naja.SNLBitTerm.is_data_input>`, :meth:`is_data_output <najaeda.naja.SNLBitTerm.is_data_output>`
   * - :class:`najaeda.naja.SNLNet` and net subclasses
     - :class:`Type <najaeda.naja.SNLNet.Type>`, :meth:`getName <najaeda.naja.SNLNet.getName>`, :meth:`setName <najaeda.naja.SNLNet.setName>`, :meth:`getDesign <najaeda.naja.SNLNet.getDesign>`, :meth:`getBits <najaeda.naja.SNLNet.getBits>`, :meth:`getWidth <najaeda.naja.SNLNet.getWidth>`, :meth:`getType <najaeda.naja.SNLBitNet.getType>`, :meth:`setType <najaeda.naja.SNLNet.setType>`, :meth:`getTypeAsString <najaeda.naja.SNLBitNet.getTypeAsString>`, :meth:`isConstant <najaeda.naja.SNLNet.isConstant>`, :meth:`isConstant0 <najaeda.naja.SNLNet.isConstant0>`, :meth:`isConstant1 <najaeda.naja.SNLNet.isConstant1>`, :meth:`getComponents <najaeda.naja.SNLBitNet.getComponents>`, :meth:`getInstTerms <najaeda.naja.SNLBitNet.getInstTerms>`, :meth:`getBitTerms <najaeda.naja.SNLBitNet.getBitTerms>`
   * - :class:`najaeda.naja.SNLInstTerm`
     - :meth:`getInstance <najaeda.naja.SNLInstTerm.getInstance>`, :meth:`getBitTerm <najaeda.naja.SNLInstTerm.getBitTerm>`, :meth:`getNet <najaeda.naja.SNLInstTerm.getNet>`, :meth:`setNet <najaeda.naja.SNLInstTerm.setNet>`, :meth:`getDirection <najaeda.naja.SNLInstTerm.getDirection>`, :meth:`getRole <najaeda.naja.SNLInstTerm.getRole>`, :meth:`getResetActiveLevel <najaeda.naja.SNLInstTerm.getResetActiveLevel>`, :meth:`is_clock <najaeda.naja.SNLInstTerm.is_clock>`, :meth:`is_async_reset <najaeda.naja.SNLInstTerm.is_async_reset>`, :meth:`is_async_set <najaeda.naja.SNLInstTerm.is_async_set>`, :meth:`is_sync_reset <najaeda.naja.SNLInstTerm.is_sync_reset>`, :meth:`is_sync_set <najaeda.naja.SNLInstTerm.is_sync_set>`, :meth:`is_reset <najaeda.naja.SNLInstTerm.is_reset>`, :meth:`is_enable <najaeda.naja.SNLInstTerm.is_enable>`, :meth:`is_data <najaeda.naja.SNLInstTerm.is_data>`, :meth:`is_data_input <najaeda.naja.SNLInstTerm.is_data_input>`, :meth:`is_data_output <najaeda.naja.SNLInstTerm.is_data_output>`
   * - :class:`najaeda.naja.SNLTermRole`
     - :attr:`Clock <najaeda.naja.SNLTermRole.Clock>`, :attr:`DataInput <najaeda.naja.SNLTermRole.DataInput>`, :attr:`DataOutput <najaeda.naja.SNLTermRole.DataOutput>`, :attr:`AsyncReset <najaeda.naja.SNLTermRole.AsyncReset>`, :attr:`AsyncSet <najaeda.naja.SNLTermRole.AsyncSet>`, :attr:`SyncReset <najaeda.naja.SNLTermRole.SyncReset>`, :attr:`SyncSet <najaeda.naja.SNLTermRole.SyncSet>`, :attr:`Enable <najaeda.naja.SNLTermRole.Enable>`, :attr:`ScanInput <najaeda.naja.SNLTermRole.ScanInput>`, :attr:`ScanEnable <najaeda.naja.SNLTermRole.ScanEnable>`, :attr:`MemoryReadAddress <najaeda.naja.SNLTermRole.MemoryReadAddress>`, :attr:`MemoryReadData <najaeda.naja.SNLTermRole.MemoryReadData>`, :attr:`MemoryWriteAddress <najaeda.naja.SNLTermRole.MemoryWriteAddress>`, :attr:`MemoryWriteData <najaeda.naja.SNLTermRole.MemoryWriteData>`, :attr:`MemoryWriteEnable <najaeda.naja.SNLTermRole.MemoryWriteEnable>`, :attr:`Other <najaeda.naja.SNLTermRole.Other>`
   * - :class:`najaeda.naja.SNLActiveLevel`
     - :attr:`High <najaeda.naja.SNLActiveLevel.High>`, :attr:`Low <najaeda.naja.SNLActiveLevel.Low>`, :attr:`NA <najaeda.naja.SNLActiveLevel.NA>`
   * - :class:`najaeda.naja.SNLPath`
     - :meth:`empty <najaeda.naja.SNLPath.empty>`, :meth:`size <najaeda.naja.SNLPath.size>`, :meth:`getInstances <najaeda.naja.SNLPath.getInstances>`, :meth:`getInstanceIDs <najaeda.naja.SNLPath.getInstanceIDs>`, :meth:`getHeadInstance <najaeda.naja.SNLPath.getHeadInstance>`, :meth:`getTailInstance <najaeda.naja.SNLPath.getTailInstance>`, :meth:`getHeadPath <najaeda.naja.SNLPath.getHeadPath>`, :meth:`getTailPath <najaeda.naja.SNLPath.getTailPath>`, :meth:`getDesign <najaeda.naja.SNLPath.getDesign>`, :meth:`getModel <najaeda.naja.SNLPath.getModel>`
   * - :class:`najaeda.naja.SNLOccurrence`
     - :meth:`isInstanceOccurrence <najaeda.naja.SNLOccurrence.isInstanceOccurrence>`, :meth:`getPath <najaeda.naja.SNLOccurrence.getPath>`, :meth:`getInstance <najaeda.naja.SNLOccurrence.getInstance>`, :meth:`getInstTerm <najaeda.naja.SNLOccurrence.getInstTerm>`, :meth:`getNetComponent <najaeda.naja.SNLOccurrence.getNetComponent>`, :meth:`getDesign <najaeda.naja.SNLOccurrence.getDesign>`
   * - :class:`najaeda.naja.SNLEquipotential`
     - :class:`Mode <najaeda.naja.SNLEquipotential.Mode>`, :meth:`getTerms <najaeda.naja.SNLEquipotential.getTerms>`, :meth:`getInstTermOccurrences <najaeda.naja.SNLEquipotential.getInstTermOccurrences>`, :meth:`isConst0 <najaeda.naja.SNLEquipotential.isConst0>`, :meth:`isConst1 <najaeda.naja.SNLEquipotential.isConst1>`, :meth:`dumpDotFile <najaeda.naja.SNLEquipotential.dumpDotFile>`
   * - :class:`najaeda.naja.NLID`
     - :meth:`from_string <najaeda.naja.NLID.from_string>`, :meth:`toTuple <najaeda.naja.NLID.toTuple>`, :meth:`getType <najaeda.naja.NLID.getType>`, :meth:`getDBID <najaeda.naja.NLID.getDBID>`, :meth:`getLibraryID <najaeda.naja.NLID.getLibraryID>`, :meth:`getDesignID <najaeda.naja.NLID.getDesignID>`, :meth:`getDesignObjectID <najaeda.naja.NLID.getDesignObjectID>`, :meth:`getInstanceID <najaeda.naja.NLID.getInstanceID>`, :meth:`getBit <najaeda.naja.NLID.getBit>`, :meth:`isDesign <najaeda.naja.NLID.isDesign>`, :meth:`isInstance <najaeda.naja.NLID.isInstance>`, :meth:`isNet <najaeda.naja.NLID.isNet>`, :meth:`isTerm <najaeda.naja.NLID.isTerm>`
   * - :class:`najaeda.naja.LogicCone`
     - :attr:`FanIn <najaeda.naja.LogicCone.FanIn>`, :attr:`FanOut <najaeda.naja.LogicCone.FanOut>`, :meth:`getDirection <najaeda.naja.LogicCone.getDirection>`, :meth:`getRoot <najaeda.naja.LogicCone.getRoot>`, :meth:`getNodes <najaeda.naja.LogicCone.getNodes>`, :meth:`getLeaves <najaeda.naja.LogicCone.getLeaves>`, :meth:`getNodeCount <najaeda.naja.LogicCone.getNodeCount>` and snake_case aliases
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

The following class and method references are generated from the compiled
extension. Building this documentation requires an importable extension from
the same checkout.

.. py:module:: najaeda.naja

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
   :inherited-members:

.. autoclass:: najaeda.naja.NLDB
   :members:
   :undoc-members:
   :inherited-members:

``NLDB.loadSystemVerilog`` writes incremental frontend diagnostics to
``naja_sv_diagnostics.log`` by default. Pass ``diagnostics_report_path=None``
to disable the report file and keep diagnostics console-only.

   Both ``NLDB.loadSystemVerilog`` and ``NLDB.loadVHDL`` accept
   ``library="DESIGN"`` to select a root ``NLLibrary`` in this database. Missing
   destinations are created. Basic names match case-insensitively, extended
   names exactly; ambiguous matches fail. Existing positional arguments retain
   their meaning. The high-level loaders expose the same keyword-only option.
   VHDL named-library references resolve only within these roots; ``work`` is
   the library owning the source unit, including imported packages. SV uses
   this option for destination storage, without cross-library source binding.

   ``NLDB.loadVHDL(file, top=None, diagnostics_report_path="naja_vhdl_diagnostics.log", library="DESIGN")`` loads one VHDL source file and returns its
   ``SNLDesign``. Load package files before their users in the same database:
   a package-only file returns ``None`` and preserves the current top design.
   A file containing one entity with required generic values also returns
   ``None`` when ``top`` is omitted: its source is retained until a parent
   supplies those values through a generic map. Pass ``top`` explicitly to
   require immediate elaboration and diagnose any missing generic values.
   Each Python call emits a ``RuntimeWarning`` because the VHDL parser is in
   Beta mode; its supported subset and behavior may change between releases.
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
   and integer/boolean generic specialization. Boolean defaults and actuals retain
   their type (integers are not implicitly converted), including across logical
   libraries. A generic can omit its default when its
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
   :inherited-members:

.. autoclass:: najaeda.naja.SNLDesign
   :members:
   :undoc-members:
   :inherited-members:

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
   :inherited-members:

.. autoclass:: najaeda.naja.SNLTerm
   :exclude-members: Direction
   :members:
   :undoc-members:
   :inherited-members:

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
   :exclude-members: Type
   :members:
   :undoc-members:
   :inherited-members:

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
   :inherited-members:

Use the raw API to declare model parameters and set instance values; these
operations do not currently have high-level ``najaeda.netlist`` wrappers.
The final value argument of ``create_decimal(design, name[, value])``,
``create_string(design, name[, value])``,
``create_boolean(design, name[, value])``, and
``create_binary(design, name, size[, value])`` is optional. Omitting it creates
a required parameter with no default. The binary creator retains its existing
``size`` argument.

.. code-block:: python

   width = naja.SNLParameter.create_decimal(model, "WIDTH")
   assert not width.hasDefaultValue()
   naja.SNLInstParameter.create(instance, width, "8")

``hasDefaultValue()`` distinguishes an absent default from an explicit zero,
false, or empty string. ``getValue()`` returns the default as a string, or
``None`` if there is none. Instance values are obtained separately with
``SNLInstParameter.getValue()``. Cloning and NajaIF snapshots preserve default
presence, including empty string defaults.

Instances may be constructed before their required values are attached. Verilog
export rejects an instance missing a required value, and always emits supplied
values for required parameters. The current Verilog writer cannot emit a model
declaration without a default and reports an error for such declarations;
it does not emit SystemVerilog parameter port lists. A model supplied externally
may still be instantiated when its declaration is excluded from the export.
Parameter-derived truth tables require a model default. Changing an instance
parameter does not re-elaborate the model's ports or topology.

.. autoclass:: najaeda.naja.SNLParameter
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLAttribute
   :members:
   :undoc-members:
   :inherited-members:

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
   :inherited-members:

.. autoclass:: najaeda.naja.SNLOccurrence
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLEquipotential
   :exclude-members: Mode
   :members:
   :undoc-members:
   :inherited-members:

   Pass ``mode=SNLEquipotential.Mode.TraverseAssigns`` to the constructor
   to cross Assign instances and omit their instance terminals.  The
   default is ``SNLEquipotential.Mode.Standard``.

.. autoclass:: najaeda.naja.SNLNetComponent
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.NLID
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.LogicCone
   :members:
   :undoc-members:
   :inherited-members:

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
   :inherited-members:

.. autoclass:: najaeda.naja.SNLActiveLevel
   :members:
   :undoc-members:
   :inherited-members:

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
