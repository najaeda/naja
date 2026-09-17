XLS IR frontend and shared elaboration study
============================================

Study plan, 2026-09-17. This document is a proposal, not a description of a
released Naja feature.

Objective
---------

Add an XLS IR frontend to Naja while turning the reusable, language-neutral
parts of the SystemVerilog lowering code into a common path for future
frontends, including VHDL.

The work has two equally important outcomes:

* Naja can construct faithful SNL designs from a useful, explicitly defined
  subset of XLS IR.
* Frontend-independent circuit construction no longer lives inside the
  SystemVerilog-specific ``SNLSVConstructorImpl`` class.

The work must preserve a strict rule already used by the SystemVerilog
frontend: an input construct is either lowered faithfully or rejected with a
clear diagnostic. An unsupported XLS operation must never become an undriven
net, a truncated value, or plausible but incorrect logic.

Executive recommendation
------------------------

Use a staged, post-language-elaboration boundary:

.. code-block:: text

   SystemVerilog -- slang elaboration --+
                                        |
   XLS IR -- XLS parse/verify ----------+--> neutral circuit operations
                                        |         and/or frontend IR
   VHDL -- VHDL-specific elaboration ---+                  |
                                                           v
                                               common SNL lowering kernel
                                                           |
                                                           v
                                                          SNL

``slang`` should continue to own SystemVerilog parsing, name and type
resolution, parameter specialization, and generate elaboration. XLS should
continue to own verification of XLS packages and, where needed, conversion of
functions or procs into scheduled block IR. A future VHDL parser should own
VHDL generics, configurations, and generate semantics.

The common Naja layer should begin after those source-language semantics have
been resolved. Calling this layer *common elaboration* is convenient, but its
precise responsibility is frontend-independent circuit lowering: typed values,
operations, ports, state, memories, instances, connectivity, names, source
provenance, and diagnostics into SNL.

Do not begin by designing a large universal HDL AST. First extract a small SNL
emission kernel from the SystemVerilog constructor and use it from one narrow
XLS vertical slice. Introduce a persistent neutral frontend IR only after the
two adapters expose the actual common semantic contract.

Research baseline
-----------------

XLS has three graph-bearing entities: functions, procs, and blocks. Functions
are stateless, procs carry recurrent state and communicate through channels,
and blocks are the RTL-level form with explicit ports, registers, and
instantiations. Blocks correspond most closely to SNL designs. See the
`XLS IR semantics <https://google.github.io/xls/ir_semantics/>`_ and
`IR overview <https://google.github.io/xls/ir_overview/>`_.

XLS also has its own hierarchy elaboration for procs and blocks. Block
elaboration follows block, FIFO, delay-line, and external-module
instantiations, and treats ``instantiation_input`` and
``instantiation_output`` nodes as hierarchy edges. See
`XLS elaboration <https://google.github.io/xls/elaboration/>`_. Naja should
consume that meaning, not independently reinterpret proc hierarchy.

The XLS code-generation flow can emit post-scheduling block IR with
``--output_block_ir_path``. This representation includes explicit timing,
registers, and ports and is the safest initial boundary for sequential designs.
See the `XLS code-generation options
<https://google.github.io/xls/codegen_options/>`_.

The native C++ entry point for textual IR is
``xls::Parser::ParsePackage`` in
`ir_parser.h
<https://github.com/google/xls/blob/main/xls/ir/ir_parser.h>`_. The parsed
``xls::Package`` retains functions, procs, blocks, types, source locations,
channels, and the selected top. XLS currently builds with Bazel, while CMake
is Naja's primary build, so dependency integration is a first-class study
question rather than a detail to defer.

XLS is explicitly experimental and changes regularly. Any direct dependency
or accepted textual contract therefore needs an exact tested revision and a
visible compatibility policy. See the
`XLS project README <https://github.com/google/xls>`_.

Current Naja baseline
---------------------

The SystemVerilog flow is currently concentrated in
``src/nl/formats/systemverilog/frontend/SNLSVConstructor.cpp``. Its main
pipeline is:

#. configure the slang driver, parse sources, and create a slang compilation;
#. validate diagnostics and obtain elaborated top instances;
#. recursively create one SNL design per specialized
   ``InstanceBodySymbol``;
#. create terms, infer memories, create nets, and connect port nets;
#. lower continuous assignments, hierarchy, combinational processes,
   sequential processes, latches, initialization, and inferred memories;
#. validate lowering coverage and reject unsupported constructs;
#. collapse safe assignment aliases and optionally retain source-AST links.

Useful common functionality exists, but it is mixed with slang objects and
SystemVerilog policy. Examples include bit-vector collection and resizing,
constant construction, primitive selection, gates, muxes, arithmetic,
comparisons, selects, DFFs, latches, memories, source annotation, generated
names, and diagnostics. By contrast, slang expression traversal, procedural
statement replay, event-control interpretation, generate traversal, and
SystemVerilog memory inference are language-specific.

This distinction defines the extraction seam:

.. list-table:: Proposed ownership
   :header-rows: 1
   :widths: 25 34 41

   * - Concern
     - Owner
     - Notes
   * - Parse, names, types, parameters/generics
     - Source adapter
     - slang, XLS, or a future VHDL implementation remains authoritative.
   * - Source hierarchy elaboration
     - Source adapter
     - Produces specialized designs or an already elaborated hierarchy.
   * - Procedural semantics
     - Source adapter
     - Converts statements or proc semantics to dataflow and next-state
       operations.
   * - Typed circuit operations
     - Common frontend contract
     - Fixed-width values, constants, combinational nodes, state, instances,
       and memories.
   * - SNL primitive emission
     - Common lowering kernel
     - Creates DB0 primitives, nets, terms, instances, and exact connections.
   * - Unsupported-feature policy
     - Shared mechanism, adapter-specific rules
     - Every rejection carries an input kind and source location when known.

Provisional implementation shape
--------------------------------

Names and directories are intentionally provisional until the first
extraction spike validates them:

.. code-block:: text

   src/nl/formats/common/frontend/
     NLFrontendSource.h        neutral source location and origin handle
     NLCircuitBuilder.h/.cpp   width-exact SNL/DB0 emission primitives
     NLFrontendIR.h/.cpp       optional normalized graph, added when justified
     NLSNLLowering.h/.cpp      normalized graph to SNL orchestration

   src/nl/formats/systemverilog/frontend/
     SNLSVConstructor.*        slang ownership and SystemVerilog semantics
     SNLSVLoweringAdapter.*    calls the common circuit API

   src/nl/formats/xls_ir/frontend/
     SNLXLSConstructor.*       public construction entry point
     SNLXLSLoweringAdapter.*   XLS entities and nodes to common operations

The first common API should operate on width-explicit bit-vector values and
return value handles, not expose slang classes, XLS classes, or raw assumptions
about bus direction. Source locations should use a small neutral value type.
Frontend-specific AST/IR handles may be stored as opaque origin metadata, but
must not become a dependency of the common library.

The common layer should cover only semantics already shared by two real
frontends. Likely first candidates are:

* constants, bit-vector width adaptation, concatenation, slicing, and
  extension;
* unary, binary, reduction, arithmetic, comparison, shift, and mux/select
  operations;
* creation and connection of terms, nets, instances, and specialized models;
* DFF/latch creation with clock, reset, enable, initialization, and source
  metadata;
* stable generated-name policy and collision handling;
* structured diagnostics and final lowering-coverage validation.

SystemVerilog-specific procedural replay and inferred-memory recognition stay
in the SV adapter. Once recognized, however, their resulting mux, state, and
memory operations should use the common emission API.

Initial XLS scope
-----------------

The frontend should state which XLS abstraction it accepts instead of treating
all ``.ir`` files as equivalent.

Milestone A: combinational function IR
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Accept a package with a selected top function and lower it as one combinational
SNL design. Initially require optimized or otherwise invocation-free IR.
Function parameters become input terms and the result becomes an output term.
Start with ``bits[N]`` values and a compact operation set sufficient for
representative arithmetic and control datapaths.

This milestone proves parsing, type/width handling, graph ordering, common
operation emission, diagnostics, and equivalence without involving scheduling.
It must document that it imports the function's combinational semantics; it
does not invent a pipeline.

Milestone B: block IR
~~~~~~~~~~~~~~~~~~~~~

Accept a selected XLS block, preferably post-scheduling block IR emitted by
XLS code generation. Map:

* input and output ports to SNL terms;
* block values and operations to common circuit operations;
* registers plus ``register_read`` and ``register_write`` to canonical DB0
  state primitives;
* reset attributes, load enables, and reset values without changing their
  priority or polarity;
* block instantiations to SNL instances and their input/output operations to
  exact connections;
* external-module instantiations to explicitly declared blackboxes.

Multiple writes to one XLS register, clock/reset conventions, and port order
must have explicit tests before this milestone is complete.

Milestone C: richer XLS types and instantiations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Define deterministic flattening for tuples and arrays, including nested and
zero-width cases. Add or explicitly reject FIFO and delay-line instantiations.
These constructs may require new canonical DB0 models or a faithful structural
expansion; they must not be represented as generic instances with guessed
behavior.

Milestone D: procs and integrated scheduling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Treat direct proc import as a later project. Proc state, channels, tokens,
flow control, activation semantics, and scheduling are not merely a different
syntax for registers. The preferred route is to use XLS's own optimization,
scheduling, and proc-to-block conversion, then import block IR. Direct proc
lowering is justified only if a concrete use case cannot use that route.

Feature matrix to maintain
--------------------------

Create a machine-readable or test-derived support table during the prototype.
At minimum it should track:

* entity kind: function, block, proc;
* type: bits, tuple, array, token;
* every XLS opcode, including side-effecting and RTL-only nodes;
* block instantiation kind: block, external, FIFO, delay line;
* register reset, enable, initialization, and multiple-write behavior;
* source-location preservation;
* handling of ``assert``, ``cover``, ``trace``, and other non-structural or
  token-dependent operations.

Each entry is ``supported``, ``rejected with diagnostic``, or ``planned``.
There is no silent ``ignored`` state. Non-synthesizable operations may be
discardable only through an explicit user option whose diagnostic policy and
observable consequences are documented.

Dependency and interchange study
--------------------------------

Evaluate these approaches with a small buildable spike:

Native XLS C++ libraries
  Use ``xls::Parser::ParsePackage`` and the verified in-memory model directly.
  This has the best semantic fidelity and access to block elaboration, but the
  transitive Abseil/protobuf/Bazel dependency graph may be difficult to expose
  through Naja's primary CMake build.

Pinned XLS bridge executable
  Build a small helper against XLS with Bazel. It parses and verifies XLS IR,
  optionally runs the official conversion to block IR, and emits a
  versioned Naja-owned interchange format. Naja's CMake build then depends only
  on that interchange schema. This isolates build systems but introduces a
  tool/version boundary and packaging work.

Independent textual parser
  Parse XLS text directly in Naja. This makes deployment simple but duplicates
  a changing grammar, verification, and operation metadata. It is not the
  preferred production design unless the accepted grammar is deliberately
  small, versioned, and differential-tested against the official parser.

Using ``codegen_main`` to produce Verilog and feeding that to the existing SV
frontend is useful as a reference oracle and temporary experiment. It is not
the new XLS frontend because it loses the opportunity to preserve XLS-level
identity and metadata and does not exercise shared Naja lowering.

The spike must finish with an architecture decision record covering:

* exact XLS revision/pinning and update policy;
* supported host platforms and binary distribution;
* CMake and Bazel behavior, including offline builds;
* license/notice propagation;
* malformed and revision-incompatible input behavior;
* whether XLS optimization or code generation is embedded, invoked, or left
  to the caller.

Phased work plan
----------------

Phase 0: evidence and contracts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Status on 2026-09-17: dependency/interchange spike completed. The isolated
artifact is in ``studies/xls_ir_frontend/``. It pins XLS commit
``0a7c502ccaaa650cb2da1929c536c4ab100eafda``, parses representative function,
block, hierarchy, tuple/array, and proc fixtures through the official public
parser, and rejects malformed IR. A Bazel-built probe and a standalone CMake
contract harness exercise the same versioned JSON boundary. The preliminary
decision is to use a pinned bridge executable for the first vertical slice;
the JSON schema is exploratory and is not yet the production interchange.

#. Collect minimal function, block, sequential block, hierarchy, tuple/array,
   external-module, and proc fixtures from or generated by a pinned XLS
   revision.
#. Run them through the official parser, verifier, interpreter, and block-IR
   code-generation path as applicable.
#. Complete the dependency spike and architecture decision record.
#. Publish the first support matrix and define exact MVP input contracts.

Exit criterion: the project can parse one fixture through the chosen boundary
in both supported build systems, and the accepted XLS entity/revision is
unambiguous.

Phase 1: extract the common SNL emission kernel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Inventory primitive-emission helpers in ``SNLSVConstructorImpl`` and mark
   their slang dependencies.
#. Introduce neutral source references, value handles, naming, and diagnostics.
#. Move a narrow set of constant, gate, mux, concatenate/slice, and arithmetic
   builders behind the common API.
#. Route the existing SystemVerilog paths through it without changing emitted
   topology.

Exit criterion: focused and regression SV tests show structural parity, and
the common library has no slang dependency.

Phase 2: first XLS vertical slice
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Add ``SNLXLSConstructor`` behind a build option if XLS support is optional.
#. Import a bits-only, combinational top function.
#. Use the common emission kernel for every supported operation.
#. Diagnose unknown operations, wrong top kinds, unsupported types, and width
   mismatches with source locations when XLS supplies them.
#. Differential-test results against the XLS interpreter over directed and
   generated vectors.

Exit criterion: representative combinational XLS functions produce equivalent
SNL/Verilog behavior without passing through generated Verilog on the import
path.

Phase 3: stabilize the shared frontend contract
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Compare the SV adapter and XLS vertical slice and record the semantic data
   actually shared.
#. Decide whether direct builder calls remain sufficient or whether a
   persistent ``NLFrontendIR`` graph adds value for validation, debugging,
   serialization, and multiple lowering passes.
#. If introduced, give the IR an invariant checker, deterministic dump format,
   and unit tests before expanding it.
#. Move shared lowering-coverage and diagnostic aggregation into the common
   layer while keeping language-specific messages in adapters.

Exit criterion: the shared contract is documented, independently testable,
and contains no source-language objects.

Phase 4: XLS block state and hierarchy
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Import ports, combinational nodes, registers, reset/enable behavior, and
   block hierarchy.
#. Use XLS block elaboration or an equivalent verified traversal rather than
   recreating hierarchy rules from text.
#. Add external-module blackboxes with explicit signatures.
#. Reject FIFO, delay-line, token, channel, or side-effecting semantics until
   each has a reviewed mapping.

Exit criterion: a scheduled multi-block XLS design with state is structurally
and behaviorally equivalent after import.

Phase 5: migrate reusable SV lowering
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Move sequential primitive emission, source-neutral width operations, and
   any proven common memory construction into the shared layer.
#. Keep slang traversal and SystemVerilog procedural/memory recognition in the
   adapter.
#. Split the current constructor by responsibility once APIs are stable; avoid
   a mechanical file split that leaves the same coupling distributed across
   more files.

Exit criterion: a meaningful second set of SV features uses the same lowering
path as XLS, with no regression in unsupported-construct detection.

Phase 6: product integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Add CMake and Bazel targets, installation/export rules, CI, and packaging.
#. Add a CLI input mode and, if exposed through Python, the corresponding raw
   and high-level API plus documentation.
#. Document top selection, accepted XLS forms/revisions, preprocessing steps,
   diagnostics, and the support matrix.
#. Establish a routine XLS pin-update test using the differential corpus.

Verification strategy
---------------------

Extraction parity
  For each helper moved out of the SV constructor, compare SNL deep structure
  before and after: model type, instance count, net widths, bit ordering,
  connectivity, constants, state metadata, and source annotations.

Differential XLS semantics
  Evaluate pure functions with the official XLS interpreter and compare them
  with simulation of the Verilog dumped from imported SNL. Include boundary
  values, random vectors, signed-operation encodings, width-one values, and
  unusual widths.

Sequential equivalence
  Compare reset, enable, and cycle-by-cycle behavior of block IR and imported
  SNL. Cover active-high/low and synchronous/asynchronous reset, hold behavior,
  and multiple register writes.

Hierarchy and types
  Assert exact port order, names, nested instance models, bit ordering, tuple
  and array flattening, and external-module signatures.

Negative coverage
  Keep one test for every rejected operation/type/instantiation kind. Tests
  assert the diagnostic category, entity/node identity, and source location,
  not only that an exception occurred.

Build coverage
  Run focused tests with CMake, then the Bazel smoke build. The primary CMake
  build must not silently gain a network or locally installed XLS dependency.

Acceptance criteria
-------------------

The initial project is complete when:

* the documented XLS function and block subsets load directly into SNL;
* supported designs match official XLS behavior on the agreed equivalence
  corpus;
* every unsupported XLS construct fails closed with a useful diagnostic;
* the SV frontend demonstrably uses a slang-free common lowering library and
  retains its regression behavior;
* CMake and Bazel pins, targets, and CI test the same XLS contract;
* adding a future adapter does not require including slang or XLS headers in
  the common layer;
* the support matrix and pin-update process are part of maintained project
  documentation.

Non-goals for the first implementation
--------------------------------------

* Parsing DSLX or C++ source; the input is XLS IR.
* Reimplementing XLS scheduling, proc activation, or channel semantics in
  Naja.
* Replacing slang's SystemVerilog elaboration.
* Designing a universal source-language AST before two adapters require it.
* Accepting an operation by approximating or dropping part of its semantics.
* Making direct proc, FIFO, delay-line, token, or assertion lowering part of
  the first MVP.

Open decisions
--------------

#. Native XLS linkage, a pinned bridge executable, or a deliberately limited
   independent parser?
#. Are both direct combinational function IR and post-scheduling block IR
   public inputs, or is block IR the only stable contract?
#. Should a neutral frontend IR be persisted, or should adapters initially call
   a checked common builder directly?
#. What is the canonical flattening and naming convention for XLS tuples and
   arrays, and how is that mapping exposed to clients?
#. Which XLS operation subset is required by the first real target designs?
#. How should XLS source locations and node IDs be represented in SNL metadata
   without coupling SNL to XLS?
#. Which external-module signature information is mandatory before creating an
   SNL blackbox?
#. Does the first release expose only C++, or also CLI and Python entry points?

Decision log
------------

2026-09-17
  Start with a post-language-elaboration boundary. Source-language parsing,
  type resolution, specialization, generate semantics, and XLS scheduling stay
  with their authoritative frontend/toolchain.

2026-09-17
  Prefer incremental extraction over a big-bang universal IR. A neutral graph
  remains the target architecture when justified by the SV and XLS adapters,
  but the first reusable artifact is a checked, slang-free SNL emission kernel.

2026-09-17
  Treat combinational functions as the smallest semantic proof and
  post-scheduling block IR as the first sequential/hierarchical contract.
  Direct proc lowering is deferred.

2026-09-17
  Complete the Phase 0 dependency spike at pinned XLS commit
  ``0a7c502ccaaa650cb2da1929c536c4ab100eafda``. Prefer a Bazel-built,
  versioned bridge for the first vertical slice because an external native
  consumer must reproduce a large dependency graph, root-only overrides,
  compiler settings, and two upstream visibility/dependency fixes. Keep
  direct linkage as a future controlled-build option.
