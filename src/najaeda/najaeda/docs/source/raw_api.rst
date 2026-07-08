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
Raw value wrappers without an ``NLID`` are intentionally unhashable.

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
     - :class:`najaeda.naja.SNLTerm`, ``SNLBitTerm``, or ``SNLInstTerm``
     - May represent top terms or instance terms.
   * - :class:`najaeda.netlist.Net`
     - :class:`najaeda.naja.SNLNet`, ``SNLBitNet``, or concatenated bits
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

* ``intent_available()``
* ``live_compilation()``
* ``ast_symbol_of(snl_object)``
* ``snl_objects_of(symbol_capsule)``
* ``intent_parameters_of(snl_object)``
* ``intent_type_of(snl_object)``
* ``intent_package_member(symbol_capsule)``

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
   * - ``NLUniverse``
     - ``create``, ``destroy``, ``get``, ``getDB``, ``getTopDB``, ``setTopDB``, ``getTopDesign``, ``setTopDesign``, ``getUserDBs``, ``getSNLDesign``, ``getObject``, ``applyDLE``, ``applyConstantPropagation``, ``getMaxFanout``, ``getMaxLogicLevel``
   * - ``NLDB``
     - ``create``, ``destroy``, ``getID``, ``getNLID``, ``isTopDB``, ``getLibraries``, ``getLibrary``, ``getGlobalLibraries``, ``getPrimitiveLibraries``, ``getTopDesign``, ``loadVerilog``, ``loadSystemVerilog``, ``loadLibertyPrimitives``, ``loadNajaIF``, ``dumpNajaIF``, ``dumpVerilog``
   * - ``NLLibrary``
     - ``create``, ``createPrimitives``, ``getDB``, ``getID``, ``getNLID``, ``getName``, ``setName``, ``isStandard``, ``isPrimitives``, ``getSNLDesign``, ``getSNLDesigns``, ``getLibrary``
   * - ``SNLDesign``
     - ``create``, ``createPrimitive``, ``clone``, ``destroy``, ``getName``, ``setName``, ``getDB``, ``getLibrary``, ``getID``, ``getNLID``, ``getRevisionCount``, ``getTerms``, ``getTerm``, ``getTermByID``, ``getScalarTerms``, ``getBusTerms``, ``getBundleTerms``, ``getNets``, ``getNet``, ``getScalarNets``, ``getBusNets``, ``getInstances``, ``getInstance``, ``getInstanceByID``, ``getInstanceByIDList``, ``getParameters``, ``getParameter``, ``dumpVerilog``, ``dumpFullDotFile``, ``dumpContextDotFile``
   * - ``SNLInstance``
     - ``create``, ``destroy``, ``getName``, ``setName``, ``getID``, ``getNLID``, ``getDesign``, ``getModel``, ``getInstTerm``, ``getInstTerms``, ``getInstParameter``, ``getInstParameters``, ``getCombinatorialInputs``, ``getCombinatorialOutputs``
   * - ``SNLTerm`` and term subclasses
     - ``Direction``, ``getName``, ``setName``, ``getDirection``, ``getDesign``, ``getNet``, ``setNet``, ``getBits``, ``getWidth``, ``getNLID``, ``getSourceLoc``, ``hasSourceLoc``
   * - ``SNLNet`` and net subclasses
     - ``Type``, ``getName``, ``setName``, ``getDesign``, ``getBits``, ``getWidth``, ``getType``, ``setType``, ``getTypeAsString``, ``isConstant``, ``isConstant0``, ``isConstant1``, ``getComponents``, ``getInstTerms``, ``getBitTerms``
   * - ``SNLInstTerm``
     - ``getInstance``, ``getBitTerm``, ``getNet``, ``setNet``, ``getDirection``, ``getRole``, ``getResetActiveLevel``
   * - ``SNLPath``
     - ``empty``, ``size``, ``getInstances``, ``getInstanceIDs``, ``getHeadInstance``, ``getTailInstance``, ``getHeadPath``, ``getTailPath``, ``getDesign``, ``getModel``
   * - ``SNLOccurrence``
     - ``isInstanceOccurrence``, ``getPath``, ``getInstance``, ``getInstTerm``, ``getNetComponent``, ``getDesign``
   * - ``SNLEquipotential``
     - ``getTerms``, ``getInstTermOccurrences``, ``isConst0``, ``isConst1``, ``dumpDotFile``
   * - ``NLID``
     - ``from_string``, ``toTuple``, ``getType``, ``getDBID``, ``getLibraryID``, ``getDesignID``, ``getDesignObjectID``, ``getInstanceID``, ``getBit``, ``isDesign``, ``isInstance``, ``isNet``, ``isTerm``
   * - ``LogicCone``
     - ``FanIn``, ``FanOut``, ``getDirection``, ``getRoot``, ``getNodes``, ``getLeaves``, ``getNodeCount`` and snake_case aliases
   * - Module functions
     - ``getVersion``, ``getGitHash``, ``setLogLevel``, ``addLogFile``, ``clearLogSinks``, ``installLoggingHandler``, ``log``, ``logInfo``, ``logWarn``, ``logCritical`` plus the SystemVerilog intent helpers listed above

Optional autodoc
----------------

When Sphinx can import the compiled extension, the directive below can expose
native docstrings.  Documentation builders that do not have the binary
available may show only the static expert reference above.

.. only:: raw_naja_available

   .. automodule:: najaeda.naja
      :members:
      :undoc-members:
      :show-inheritance:
