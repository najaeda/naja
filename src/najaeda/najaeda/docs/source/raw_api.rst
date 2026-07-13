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
and reusable scripts because it works with bundled package layouts too.

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
     - ``create``, ``destroy``, ``getID``, ``getNLID``, ``isTopDB``, ``getLibraries``, ``getLibrary``, ``getGlobalLibraries``, ``getPrimitiveLibraries``, ``getTopDesign``, ``loadVerilog``, ``loadSystemVerilog``, ``loadLibertyPrimitives``, ``loadNajaIF``, ``dumpNajaIF``, ``dumpVerilog``
   * - :class:`najaeda.naja.NLLibrary`
     - ``create``, ``createPrimitives``, ``getDB``, ``getID``, ``getNLID``, ``getName``, ``setName``, ``isStandard``, ``isPrimitives``, ``getSNLDesign``, ``getSNLDesigns``, ``getLibrary``
   * - :class:`najaeda.naja.SNLDesign`
     - ``create``, ``createPrimitive``, ``clone``, ``destroy``, ``getName``, ``setName``, ``getDB``, ``getLibrary``, ``getID``, ``getNLID``, ``getRevisionCount``, ``getTerms``, ``getTerm``, ``getTermByID``, ``getScalarTerms``, ``getBusTerms``, ``getBundleTerms``, ``getNets``, ``getNet``, ``getScalarNets``, ``getBusNets``, ``getInstances``, ``getInstance``, ``getInstanceByID``, ``getInstanceByIDList``, ``getParameters``, ``getParameter``, ``getClockTerms``, ``getAsyncResetTerms``, ``getAsyncSetTerms``, ``getSyncResetTerms``, ``getSyncSetTerms``, ``getDataInputTerms``, ``getOutputTerms``, ``setTruthTable``, ``setTruthTables``, ``getTruthTable``, ``getTruthTableByOutputID``, ``isConst0``, ``isConst1``, ``isConst``, ``isBuf``, ``isInv``, ``isAnd``, ``isNand``, ``isOr``, ``isNor``, ``isXor``, ``isXnor``, ``isMux``, ``dumpVerilog``, ``dumpFullDotFile``, ``dumpContextDotFile``
   * - :class:`najaeda.naja.SNLInstance`
     - ``create``, ``destroy``, ``getName``, ``setName``, ``getID``, ``getNLID``, ``getDesign``, ``getModel``, ``getInstTerm``, ``getInstTerms``, ``getInstParameter``, ``getInstParameters``, ``getCombinatorialInputs``, ``getCombinatorialOutputs``
   * - :class:`najaeda.naja.SNLTerm` and term subclasses
     - ``Direction``, ``getName``, ``setName``, ``getDirection``, ``getDesign``, ``getNet``, ``setNet``, ``getBits``, ``getWidth``, ``getNLID``, ``getSourceLoc``, ``hasSourceLoc``
   * - :class:`najaeda.naja.SNLNet` and net subclasses
     - ``Type``, ``getName``, ``setName``, ``getDesign``, ``getBits``, ``getWidth``, ``getType``, ``setType``, ``getTypeAsString``, ``isConstant``, ``isConstant0``, ``isConstant1``, ``getComponents``, ``getInstTerms``, ``getBitTerms``
   * - :class:`najaeda.naja.SNLInstTerm`
     - ``getInstance``, ``getBitTerm``, ``getNet``, ``setNet``, ``getDirection``, ``getRole``, ``getResetActiveLevel``, ``is_clock``, ``is_async_reset``, ``is_async_set``, ``is_sync_reset``, ``is_sync_set``, ``is_reset``, ``is_enable``, ``is_data``, ``is_data_input``, ``is_data_output``
   * - :class:`najaeda.naja.SNLTermRole`
     - ``Clock``, ``DataInput``, ``DataOutput``, ``AsyncReset``, ``AsyncSet``, ``SyncReset``, ``SyncSet``, ``Enable``, ``MemoryReadAddress``, ``MemoryReadData``, ``MemoryWriteAddress``, ``MemoryWriteData``, ``MemoryWriteEnable``, ``Other``
   * - :class:`najaeda.naja.SNLPath`
     - ``empty``, ``size``, ``getInstances``, ``getInstanceIDs``, ``getHeadInstance``, ``getTailInstance``, ``getHeadPath``, ``getTailPath``, ``getDesign``, ``getModel``
   * - :class:`najaeda.naja.SNLOccurrence`
     - ``isInstanceOccurrence``, ``getPath``, ``getInstance``, ``getInstTerm``, ``getNetComponent``, ``getDesign``
   * - :class:`najaeda.naja.SNLEquipotential`
     - ``getTerms``, ``getInstTermOccurrences``, ``isConst0``, ``isConst1``, ``dumpDotFile``
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
      SNLTermRole
      SNLActiveLevel

   .. autoclass:: najaeda.naja.SNLTerm.Direction
      :members:
      :undoc-members:

   .. autoclass:: najaeda.naja.SNLNet.Type
      :members:
      :undoc-members:

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
