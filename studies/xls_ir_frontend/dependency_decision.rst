Phase 0 dependency decision: use a pinned XLS bridge
====================================================

:Status: Proposed for the first vertical slice
:Date: 2026-09-17
:XLS revision: ``0a7c502ccaaa650cb2da1929c536c4ab100eafda``

Context
-------

Naja's production build and packaging flow is CMake-first. XLS is Bazel-first
and its root module resolves a broad dependency graph, including Python,
Abseil, protobuf, gRPC, LLVM-related repositories, OR-Tools, and HDL tooling.
This remains true when a consumer asks only for the IR parser target.

The pinned revision exposes ``xls/public/ir_parser.h`` and the
``@xls//xls/public:ir_parser`` target. That makes a native C++ integration
technically possible and avoids using the private ``xls::Parser`` class.
It does not make XLS a small or CMake-native dependency. In this external
consumer spike, the target required about 40,000 configured Bazel targets and
2,857 initial build actions on macOS arm64.

The pinned XLS module is not directly consumable as an external dependency
without compatibility work. The spike had to:

* repeat root-only overrides for unversioned ``rules_hdl``, OpenROAD, and
  related repositories;
* promote dev-only LLVM and compile-database dependencies that are referenced
  while loading XLS packages;
* make the nominally public parser target externally visible; and
* mirror XLS's Bazel 8.7, C++20, and empty-glob settings.

Those adjustments are isolated under this study's ``MODULE.bazel``,
``.bazelrc``, and ``patches/`` directory. They are evidence, not patches that
Naja should silently maintain in its production build.

Decision
--------

Use a small Bazel-built bridge executable for the first Naja XLS vertical
slice. The bridge must:

* use XLS's public parser and verifier at an exact tested commit;
* accept textual XLS IR and fail closed on malformed or incompatible input;
* emit a Naja-owned, explicitly versioned interchange schema;
* preserve entity kind, names, types, node IDs, operands, and source
  provenance needed by diagnostics;
* report its XLS revision in every payload;
* leave optimization, scheduling, and proc-to-block conversion to an explicit
  caller-selected XLS step rather than running them implicitly.

Naja's CMake targets should consume the bridge output and must not directly
link XLS or inherit its dependency graph. Bazel may build and test the bridge,
while CMake contract tests receive the executable path as a tool dependency.

The probe in this directory demonstrates that boundary with JSON schema
version 1. JSON is convenient for the study, not the final schema choice.
Cap'n Proto is a natural production candidate because Naja already uses it,
but the schema must remain Naja-owned and independent from XLS C++ layouts.

Consequences
------------

The bridge keeps the default Naja build offline and free of an XLS toolchain.
It also makes the accepted input revision visible and gives future frontends a
language-neutral handoff point. Packaging must nevertheless supply a matching
bridge binary when XLS import is enabled, and protocol compatibility becomes
a maintained contract.

Direct native linkage remains an allowed future optimization for controlled
build environments. It should be reconsidered only if XLS offers a smaller,
stable, distributable parser library or Naja adopts a supported Bazel product
build. An independent parser is rejected for now because it would duplicate
XLS grammar and verification semantics.

Validation result
-----------------

The Bazel probe parsed bits-only function, registered block, block hierarchy,
tuple/array, and proc fixtures and rejected an unresolved operand with an XLS
``INVALID_ARGUMENT`` diagnostic. The CMake harness then ran the same contract
test against the Bazel-built executable without acquiring or linking XLS.
This satisfies the Phase 0 build-boundary experiment; it does not yet lower
any XLS node into SNL.

Update policy
-------------

An XLS pin update is a reviewed compatibility change. Update the commit in
``MODULE.bazel``, the probe's reported revision, and the contract test
together. Run every fixture plus negative malformed-input tests, inspect the
interchange diff, and add a support-matrix row for any new operation or type.
No floating branch or unreported runtime XLS revision is accepted.

Licensing and platforms
-----------------------

XLS is Apache-2.0. A distributed bridge must retain the XLS license and all
notices for linked dependencies. The initial supported platform set should be
limited to platforms on which the pinned XLS bridge is built and exercised by
Naja CI; the study does not claim binary portability beyond that set.
