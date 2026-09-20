# VHDL frontend boundary: first feasibility contract

Status: proposed semantic/IR contract, with a small implemented backend extraction.
See [the study](vhdl_frontend_study.md) for milestones and required repository
extraction. Names below describe responsibilities, not existing public APIs.

## Repository ownership

`src/vhdl/` is the self-contained future frontend repository. It owns source
buffers, tokens/syntax, diagnostics, declaration/type identity, VHDL constants,
name resolution, package/library services and language-specific specialization.
No public or private dependency on Naja or SNL is allowed. Its tests must run
from a copy of this directory without the Naja tree.

`src/nl/formats/hdl/` is shared Naja hardware construction for both languages.
It deliberately depends on SNL and stays in Naja. Future common elaboration/IR
code under `src/hdl/` also stays in Naja. `hdl` names only shared infrastructure;
VHDL-specific code and eventual integration use `vhdl`.

Naja translates the frontend semantic model through its VHDL adapter. Frontend
headers do not mention Naja hardware IR. Naja owns the elaboration coordinator,
the concrete instance graph and the backend; the frontend supplies the language
rules necessary to resolve and specialize each requested instance.

## Semantic model handed to the adapter

An analysis session owns immutable syntax, source buffers and semantic objects.
Handles are valid only within that session and compilation revision. The adapter
either retains that owner or copies the information it needs; raw pointers cannot
outlive it. Different sessions must not share mutable declaration caches.

An elaboration request identifies a logical library, entity, optional architecture
or configuration, typed generic bindings, revision, and binding environment.
Naja asks the frontend to resolve those identities and evaluate required static
expressions. A specialization cache key includes declaration revision, selected
architecture/configuration, canonical typed generic values, package dependencies,
language revision and semantic options. Identical text or generic values alone
are not sufficient identities.

Values retain declared type identity and subtype constraints. Arrays retain each
dimension's left/right bounds, direction and length, including null ranges.
Records retain ordered field identities; enumerations retain declared literals.
Static integers use sufficient precision to evaluate before a range check. Logic
constants retain all language values; no unknown/weak value silently becomes 0.
Overloads and intrinsic recognition use resolved declarations and signatures.

Analysis failures contain stable diagnostic codes, source ranges and related
declarations. Unsupported hardware is a distinct adapter diagnostic. A recovered
syntax tree is usable for tooling, but not authorization to emit hardware.

## Initial common hardware IR

Start with constants, ports, values, explicit bit extraction/concatenation,
conversions, muxes, primitive arithmetic, connections, instances and registers.
Each operation has explicit input/result widths and a declared value domain.
Type analysis inserts all conversions before lowering; the backend does not
guess source-language sizing rules.

Flattened bit vectors use bit position zero as the least significant hardware
bit. The adapter records a separate mapping from source indices to positions.
For a one-dimensional vector interpreted by position, the rightmost source
element maps to bit zero regardless of ascending/descending direction. Record
field placement is explicit. Empty source arrays never create width-zero SNL
primitives: keep them in semantic shape metadata and eliminate only operations
whose semantics justify it, otherwise diagnose unsupported lowering.

Each register has current value, next value, clock edge, optional reset and
enable, priority, and initialization. These are normalized descriptions, not
source statements. Immediate variable updates, current signal reads and scheduled
signal writes use separate environments. For example, `s <= d; q <= s;` produces
`next(s)=d, next(q)=current(s)`, while `v := d; q <= v;` uses the updated variable.
Source order and overlapping partial writes must be resolved before backend entry.

The initial hardware domain is binary. Analysis retains richer values, but
hardware lowering must reject dependencies on resolution/strength/event behavior
outside the chosen profile. Standard edge functions cannot be replaced by textual
name matching. Initialization has its own representation and must not be discarded.

Provenance is a neutral source span plus declaration/instance-path identity.
Naja owns translation into SNL metadata. The frontend has no callback that creates
SNL objects and no dependency on Naja's live AST registry.

## Implemented construction proof

`SNLRTLPrimitives::createMux()` takes a design, select bit, two explicit input-bit
vectors and an existing output net. It preserves the input vector ordering and
the output net's declared ordering, and returns the created canonical primitive
instance. `createDFF()` takes a design and width-one clock/data/output nets and
returns a positive-edge register. Both validate net ownership and widths before
creating an instance. Neither receives source AST objects or reads active frontend
process state.

The existing SV vector-mux and positive-edge-register helpers now call these
shared functions. They still perform language-specific normalization, source
annotation, retained AST binding and initialization attachment. Direct C++ tests
exercise the same interface without any language compiler dependency, including
reordered inputs, ascending/nonzero outputs and rejection without partial instance
creation. This proves a narrow backend boundary; it is not a VHDL adapter or a
complete shared elaborator.

Precondition rejection is atomic for these two builders. Full construction is
not yet transactional: allocation failures, metadata failures and other frontend
paths can still leave partially constructed state. The future coordinator must
stage a complete design in a privately owned database/library and publish it only
after validation; exact ownership and rollback integration remain Phase 1 work.

## Acceptance and remaining feasibility work

This first increment must pass the pinned reference probes, runner failure tests,
direct primitive tests and existing SV constructor regressions. Preserve source
metadata and initialization via the existing intent/sequential tests. Run the
reference bundle from a standalone copy to establish its extraction boundary.

Phase 0 evidence now includes pinned UART, RPU and NEORV32 revisions and scopes in
`src/vhdl/tests/corpus/manifest.json`, plus a 35-probe static-analysis comparison
in `src/vhdl/tests/semantic/reuse/vhdl_lang_0.88.0.json`. GHDL imported and
resolved the UART core (VHDL-1993), RPU core (VHDL-2008), and all 61 NEORV32
RTL VHDL files (VHDL-2008). Review corpus test rights before CI use.

The comparison frontend matched the expected analysis acceptance/rejection for
all 35 cases. It correctly accepts three sources whose expected failures occur
only during elaboration or simulation; this is not a runtime comparison. Its
Rust API has no C ABI or hardware-lowering interface, so adoption would need an
FFI or process boundary and more study of generic specialization. NVC installation
could not proceed because the available Apple toolchain requires an unavailable
Xcode license; GHDL is the only runtime oracle exercised so far. The GHDL archive,
standard-package source hashes and Apache-2.0 package notices are recorded in
`src/vhdl/tests/semantic/reference.json`.

Before declaring Phase 0 complete: review corpus use rights, select a second
runtime oracle, confirm the binary hardware profile against the corpus, and
record the build/reuse decision. The handwritten C++ frontend remains a
provisional working direction; these experiments do not settle the decision.

## Validation of this increment

On 2026-09-20, GHDL 6.0.0 passed all 35 probes, including seven expected
rejections. The same 35 probes passed from a copy of `src/vhdl/` outside the
repository. Five runner tests passed, including checks against false passes from
timeouts, compiler crashes, wrong diagnostics and missing completion markers.
The tested compiler archive and standard-package source hashes are recorded in
`src/vhdl/tests/semantic/reference.json`.

The CMake Release build passed 1,116 tests: four direct shared-builder tests and
1,112 existing SV constructor/intent tests. The production frontend target also
built. Validation used the installed Homebrew LLVM 23.1.0 in
`build-vhdl-feasibility/`; the default local Apple toolchain was unavailable.
Commands after configuring a working C++ toolchain:

```sh
cmake --build build-vhdl-feasibility --target snlRTLPrimitivesTests snlSVConstructorTests naja_snl_systemverilog -j 4
ctest --test-dir build-vhdl-feasibility -R 'SNLRTLPrimitivesTest|SNLSVConstructor|SNLSVIntent' --output-on-failure -j 6
python3 -m unittest discover -s src/vhdl/tests/semantic -p test_runner.py
python3 src/vhdl/tests/semantic/run.py --ghdl /path/to/ghdl --report /tmp/vhdl-results.json
```

The new Bazel target was attempted, but the local LLVM/Apple SDK combination
failed Bazel's external-header validation on `SDKSettings.json` while compiling
the test framework. No Bazel test ran; this remains an environment validation
limitation rather than a passing smoke result.
