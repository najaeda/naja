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
returns a positive-edge register. `createGate()` takes a canonical scalar gate
kind, explicit width-one inputs and an existing output net. All three validate
net ownership, widths and arity before creating an instance. None receives source
AST objects or reads active frontend process state.

The existing SV vector-mux and positive-edge-register helpers now call these
shared functions. The VHDL adapter uses the same gate models for its typed scalar
logical expressions. The language adapters still perform language-specific normalization, source
annotation, retained AST binding and initialization attachment. Direct C++ tests
exercise the same interface without any language compiler dependency, including
reordered inputs, ascending/nonzero outputs and rejection without partial instance
creation. This proves a narrow backend boundary; it is not a VHDL adapter or a
complete shared elaborator.

Precondition rejection is atomic for these two builders. The one-level hierarchy
adapter also rolls back every leaf and top design created by its call. General
construction is not yet transactional: allocation failures, metadata failures
and other frontend paths can still leave partially constructed state. A future
coordinator should stage a complete design in a privately owned database/library
and publish it only after validation.

## Acceptance and remaining feasibility work

This first increment must pass the pinned reference probes, runner failure tests,
direct primitive tests and existing SV constructor regressions. Preserve source
metadata and initialization via the existing intent/sequential tests. Run the
reference bundle from a standalone copy to establish its extraction boundary.

Phase 0 evidence now includes pinned UART, RPU and NEORV32 revisions and scopes in
`src/vhdl/tests/corpus/manifest.json`, plus a 35-probe static-analysis comparison
in `src/vhdl/tests/semantic/reuse/vhdl_lang_0.88.0.json`. GHDL imported and
resolved the UART core (VHDL-1993), RPU core (VHDL-2008), and all 61 NEORV32
RTL VHDL files (VHDL-2008). Repository-level licenses for these references are
MIT, Apache-2.0 and BSD-3-Clause; the review and notice conditions are recorded
in the corpus manifest. No upstream RTL is vendored. Verify notices at the
exact pinned revisions before copying source into distributions.

The comparison frontend matched the expected analysis acceptance/rejection for
all 35 cases. It correctly accepts three sources whose expected failures occur
only during elaboration or simulation; this is not a runtime comparison. Its
Rust API has no C ABI or hardware-lowering interface, so adoption would need an
FFI or process boundary and more study of generic specialization. GHDL 6.0.0
and NVC 1.23.0 have now both run the semantic corpus; reproducible runners,
versioned reports and NVC-specific diagnostic/stage expectations are under
`src/vhdl/tests/semantic/`. The GHDL archive, standard-package source hashes
and Apache-2.0 package notices are recorded in
`src/vhdl/tests/semantic/reference.json`.

Before declaring Phase 0 complete: confirm the binary hardware profile against
the corpus and record the build/reuse decision. Repository-level corpus rights
are reviewed for reference/test use; inspect exact pinned-file notices before
redistributing source. The
handwritten C++ frontend remains a provisional working direction; these
experiments do not settle the decision.

## Validation of this increment

On 2026-09-20, GHDL 6.0.0 passed all 35 probes, including seven expected
rejections. The same 35 probes passed from a copy of `src/vhdl/` outside the
repository. GHDL and NVC 1.23.0 each pass all 35 probes with the pinned
expectation sets. Eight runner tests check against false passes from timeouts,
compiler crashes, wrong diagnostics and missing completion markers.
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

## Scalar clocked scheduling proof

The Phase 1 adapter accepts one positive-edge process over scalar `bit`
ports and internal scalar `bit` signals, including grouped declarations:

```vhdl
entity pipeline is
  port (clk, d : in bit; q : out bit);
end;
architecture rtl of pipeline is
  signal stage : bit;
begin
  process(clk) begin
    if clk'event and clk = '1' then
      stage <= d;
      q <= stage;
    end if;
  end process;
end;
```

The clock may use `rising_edge(clk)` or the equivalent
`clk'event and clk = '1'` guard. The sensitivity, event and level names must
resolve to the same input port.
Each assignment targets a distinct output port or internal signal; its RHS is
an input port, internal signal or assigned local variable name (parentheses are accepted). Every internal
signal must have exactly one assignment in this process. Basic names are case
insensitive. Output-port reads remain unsupported. Multiple processes and mixing
concurrent assignments with the process are rejected, even for disjoint drivers.

The standalone parser retains declaration groups, ordered assignments and source
spans; the analyzer resolves every internal name and diagnoses duplicates and
missing declarations without importing Naja/SNL. The adapter validates all types,
names, modes and supported driver patterns before creating any design. It creates
all signal nets first, then calls `SNLRTLPrimitives::createDFF()` for each write.
Signal reads connect to the current signal net; local variable reads use the
immediate-value environment described below.
Consequently `q` captures the previous `stage` value in either source order.
Repeated writes to one target are rejected rather than implementing VHDL's
last-write scheduling rule incorrectly.

`PipelineConnectivityAndCycles` verifies both canonical DFF models, shared clock,
and data/output connectivity, reversed writes, case variations, parentheses and
multiple internal signals. A small test evaluator samples all canonical DFF data
nets together. `VHDLPipelineReference` compares its output against NVC for both
source orders using the same `pipeline.vhd` fixture and eight input cycles; the
reference also checks stability on data changes and falling edges. CMake enables
this comparison when NVC and Python are available. From the Naja repository root, run it explicitly with:

```sh
python3 test/nl/formats/vhdl/compare_pipeline.py --nvc /path/to/nvc \
  --adapter build-vhdl-feasibility/test/nl/formats/vhdl/snlVHDLConstructorTests
```

This proof promises clocked behavior after the pipeline fills, not power-up
state equivalence. VHDL `bit` defaults to `'0'`; the canonical hardware DFF has
no initialization guarantee. The evaluator begins with unknown register values
and the reference comparison starts after two rising edges. Explicit initializers
(including `'0'`) are rejected. No initialization metadata is silently discarded.

Unsupported forms include asynchronous reset or nested control flow beyond the
supported active-high enable and synchronous reset-to-zero profiles, falling-edge
registers, active-low controls, unassigned
retained variables, repeated targets, multiple drivers, undriven internal
signals, timing/waveforms (`after`, `transport`, `reject`, `wait`), vector and
nine-valued types, clocked assignment expressions beyond names, and function
calls other than the supported `rising_edge` clock guard. Parser errors or adapter diagnostics reject these before design
publication. This remains a narrow scheduling proof; vectors and one-level
hierarchy are covered separately below, while general type analysis and
source-rich adapter diagnostics remain future work.

Validation (2026-09-21): 29 focused CMake tests passed in
`build-vhdl-feasibility`, including the NVC 1.23.0 reference comparison for both
write orders and the existing SV mux/register checks. All 18 standalone tests
passed from a temporary copy, followed by installation and a separate client
linked only to the exported `vhdl::frontend` target. The local LLVM build used
`-DCMAKE_CXX_SCAN_FOR_MODULES=OFF` to avoid a stale dependency-scanner path.

## Immediate process-variable proof

The clocked scalar proof accepts process-local `variable` declarations and
ordered `:=` assignments. Variables assigned before every read in the same
activation remain temporary values. For example:

```vhdl
process(clk)
  variable temp, copy : bit;
begin
  if clk'event and clk = '1' then
    stage <= d;
    temp := stage;
    copy := temp;
    delayed <= copy;
    temp := d;
    immediate <= temp;
    captured <= copy;
    copy := d;
  end if;
end process;
```

`delayed` and `captured` receive the previous `stage`; `immediate` receives `d`
on this edge. Reassigning `temp` does not change the value already copied into
`copy`, and reassigning `copy` does not change an earlier scheduled signal RHS.
The four signal destinations use canonical DFFs; these temporary variables
introduce no nets or additional storage.

The standalone AST distinguishes signal and variable assignments and preserves
source order, declaration groups and spans. Name analysis checks local scope,
duplicates and assignment object class (`<=` for signals, `:=` for variables).
`Analyzer::schedule()` uses a separate immediate-value environment and returns
frozen source/target names for signal writes. The adapter validates scalar types
and input/internal-signal reads before creating the design, then consumes that
schedule through the shared DFF builder. No Naja/SNL dependency was added to the
frontend.

VHDL process variables that are read before assignment use the retained-state
profile below. Explicit initializers, shared variables, local shadowing, non-`bit`
types, control flow and timing remain rejected. Variable reassignment is allowed;
repeated signal targets remain unsupported. Reads of output ports remain outside
this proof. Variable-only processes without a scheduled signal write are rejected.

`VariableSchedulingConnectivityAndCycles` checks the four DFFs and compares a
simultaneous-sampling evaluator against the shared `variables.vhd` fixture run
by NVC. The testbench checks immediate and delayed values and stability between
rising edges. `VHDLVariableReference` is enabled alongside the pipeline reference
when NVC and Python are available. Comparisons retain the existing post-fill
boundary; hardware power-up state is not promised.

Variable-proof validation (2026-09-21): all 37 focused CMake tests passed,
including pipeline and variable comparisons against NVC 1.23.0. All 23 standalone
frontend tests passed from a temporary copy. An installed-target client also
parsed, analyzed and scheduled `v := d; q <= v;` using only `vhdl::frontend`.

## Retained process-variable proof

A scalar `bit` variable read before its first assignment in an activation is now
identified as retained state. For `q <= retained; retained := d;`, the scheduler
freezes `q`'s source as the variable's current value and emits one final state
write for `retained`. The adapter creates a private scalar net and canonical DFF
for that variable, alongside the DFF for `q`; temporary variables still create no
storage. Retained variables must be assigned on every supported activation.
Their final next value must resolve to a non-retained scalar name; retained-state
self-feedback and cycles remain rejected because implicit initialization is not
yet represented in hardware.

`RetainedVariableConnectivityAndCycles` checks both DFFs and simultaneous edge
sampling. `VHDLRetainedVariableReference` compares the post-fill output sequence
and between-edge stability with NVC 1.23.0. Implicit VHDL `bit` initialization is
not claimed: the hardware evaluator begins with unknown state and comparison starts
after the retained pipeline has filled. Initializers, conditional state updates,
non-name expressions, and retained variable state of non-`bit` types remain
rejected before design publication.

Retained-state validation (2026-09-21): all 40 focused lexer, parser, analyzer,
adapter and NVC reference tests passed. All 24 standalone tests also passed from
an isolated copy, followed by installation and a separate client linked only to
the exported `vhdl::frontend` target.

## Typed scalar expression proof

The standalone analyzer records narrow scalar types independently of Naja and
checks assignment compatibility, conditions, equality and the scalar logical
operators. The adapter consumes those checked types and lowers nested scalar
`bit` logic through the shared canonical gate builder. Conditional branches may
contain the same expressions; the supported select condition remains `name = '1'`.
Equivalent VHDL and SystemVerilog fixtures use the same canonical And, Not and
Xor models. Unsupported operators, mismatched scalar types and non-binary
character literals are rejected before the adapter publishes a design.

Scalar-expression validation (2026-09-21): all 52 focused lexer, parser,
analyzer, shared-primitive, adapter, SystemVerilog and NVC reference tests
passed. All 26 standalone tests passed from an isolated copy; installation and
a separate client using `ScalarType` through only `vhdl::frontend` also passed.

## Constrained vector proof

The frontend now retains bounds and direction for one-dimensional constrained
`bit_vector` expression types and uses equal length, rather than equal index
labels or direction, for positional assignment compatibility. The adapter keeps
source bounds on SNL terms and nets while normalizing the rightmost source
element to hardware position zero at the shared primitive boundary.

`SNLRTLPrimitives::createBitwiseGate()` validates equal nonzero widths before
creating one canonical scalar gate per hardware position. Vector mux inputs use
the same least-significant-bit-first convention. Unconstrained and non-binary
vectors, null ranges, mismatched lengths, vector clocked state and vector selection operations
remain unsupported and fail before design publication.

Vector validation (2026-09-21): all 59 focused frontend, primitive, adapter,
SystemVerilog and NVC reference tests passed. All 27 standalone tests passed
from an isolated copy, followed by installation and a separate client reading
the preserved vector range through only `vhdl::frontend`.

## One-level hierarchy construction proof

The standalone syntax model represents labeled direct entity instantiations and
their positional name actuals. Analysis resolves `work` entities and validates
labels, arity, actual declarations, and equal scalar/vector widths without an
SNL dependency. The Naja adapter entry point
`VHDLConstructor::construct(source, top)` requires an explicit top and lowers a
structural top over behavioral leaf designs. Whole terms and nets are connected
by position, so differing legal vector bounds and directions remain faithful.

This boundary deliberately excludes named or `open` associations, component and
configuration binding, nested hierarchy, multiple architectures for a used
entity, mixed behavioral/structural top bodies, and multiple or missing drivers.
All such shapes fail before design publication; construction exceptions destroy
the top and leaf designs created during the call.

Hierarchy validation (2026-09-21): all 60 VHDL lexer, parser, analyzer,
adapter and NVC reference tests passed in the integrated build. All 31
standalone frontend tests passed from a separate build tree.

## Standard rising-edge clock proof

`ClockedProcess` distinguishes the explicit event-and-level guard from the
standard `rising_edge(clk)` call. Both resolve one scalar `bit` clock and lower
through the same canonical DFF construction; the source form is not discarded
by parsing. Only a single name argument is accepted. The pipeline NVC comparison
now exercises the call form, while direct adapter tests retain both spellings.
The synchronous reset profile is covered below. Asynchronous reset,
falling-edge, active-low control and general function-call semantics remain
outside this increment and fail before design publication.

Rising-edge validation (2026-09-21): all 63 integrated VHDL tests passed,
including the NVC pipeline comparison. All 33 standalone frontend tests passed
from a separate build tree.

## Active-high clock-enable proof

The accepted extension to a positive-edge process is exactly one nested
`if en = '1' then ... end if;` around all scheduled writes. `ClockedProcess`
retains the enable name and literal. Analysis resolves the name as a scalar
`bit`; construction further requires an input port and the active-high literal
`'1'` before publishing any design.

Each enabled write lowers through `SNLRTLPrimitives::createDFFE()` to the
canonical NLDB0 DFFE C/D/E/Q pins. The same enable gates every scheduled signal
and retained-variable state element, so a false enable holds state without a
feedback mux. The `enabled.vhd` NVC fixture checks enabled update, disabled hold
and re-enabled update and produces `1, 1, 0` in both implementations. Active-low
enables, deeper nesting and other enable forms remain outside the contract and
are rejected before design publication. Reset-priority combination is accepted
only by the profile below.

Clock-enable validation (2026-09-22): all 68 integrated VHDL tests passed,
including the NVC comparison; all 36 standalone frontend tests passed. The
shared primitive and SystemVerilog regression passed all 1,119 tests.

## Active-high synchronous reset proof

The accepted reset shape is exactly one nested
`if rst = '1' then ... else ... end if;` inside a supported positive-edge
process. The AST retains the reset name, level and reset assignments. Analysis
binds a scalar `bit` reset and type-checks both branches. Construction requires
an input reset port, a high-active condition, and a reset branch assigning `'0'`
to exactly the same scalar signal targets as the data branch.

Every resulting state element uses `SNLRTLPrimitives::createDFFSR()` and the
canonical NLDB0 DFFSR C/D/R/Q pins. The `reset.vhd` reference demonstrates
synchronous behavior: asserting reset while the clock is low preserves Q until
the rising edge, producing the trace `1, 1, 0, 1`; a two-target constructor case
checks that one reset net controls all scheduled writes. Active-low,
asynchronous, reset-to-one and process-variable forms are rejected before design
publication.

Synchronous-reset validation (2026-09-22): all 72 integrated VHDL tests passed,
including the NVC comparison; all 38 standalone frontend tests passed. The
shared primitive and SystemVerilog regression passed all 1,120 tests.

## Synchronous reset with active-high clock enable

The accepted combined shape is exactly
`if rst = '1' then ... elsif en = '1' then ... end if;` inside a supported
positive-edge process. `ClockedProcess` retains both controls and both branches,
analysis resolves each control as scalar `bit`, and construction requires input
ports with active-high levels. The reset branch retains reset priority and the
same reset-to-zero, equal-target-set invariant as the reset-only profile.

Every resulting state element uses `SNLRTLPrimitives::createDFFSRE()` and the
canonical NLDB0 DFFSRE C/D/E/R/Q pins. The `reset_enable.vhd` reference checks
enabled update, disabled hold, synchronous timing, reset-over-enable priority
and re-enabled update, producing `1, 1, 1, 0, 0, 1`. Active-low controls, an
additional else branch, deeper control flow and process-variable reset are
rejected before design publication.

Combined reset-enable validation (2026-09-22): all 81 focused VHDL/frontend and
shared-primitive tests passed, including all nine NVC comparisons. The broad
shared primitive and SystemVerilog regression passed all 1,137 tests.

## Library context and nine-valued analysis foundation

`ContextClause` is attached independently to each parsed entity and architecture.
It retains `library` clauses and the complete selected names from `use` clauses;
context never implicitly carries into the following design unit. The current
analyzer recognizes whole-package imports of `ieee.std_logic_1164.all` and
`ieee.numeric_std.all`. Missing library clauses, partial imports and unsupported
packages are deterministic diagnostics.

`std_logic` and constrained `std_logic_vector` resolve only when
`std_logic_1164` is visible and remain distinct from `bit` and `bit_vector` in
the semantic type system. Typed character literals retain the nine-value domain,
and logical/equality operations preserve the imported type. This is analysis
support, not a binary-lowering policy: the Naja adapter still rejects all
`std_logic` hardware before publication. `numeric_std` package visibility is
represented, while its declared types and overloads are not yet resolved.

Context/type validation (2026-09-22): all 76 VHDL tests passed—36 integrated
constructor/NVC tests and 40 standalone parser/analyzer tests. The previously run
1,137-test shared primitive/SystemVerilog regression remains unchanged because
this slice touches only the VHDL syntax and semantic layers.

## `numeric_std` vector arithmetic analysis foundation

With `ieee.numeric_std.all` visible, constrained `unsigned` and `signed` are
distinct semantic types. Matching vector operands support the `+` and `-`
overloads; the result has the maximum operand length and the canonical range
`length - 1 downto 0`. The importing context belongs to the design unit, so an
architecture using an overload must import it independently of its entity.

This increment does not silently treat numeric arrays as binary SNL buses.
Mixed signedness, invisible overloads, unconstrained or null arrays and
unimplemented overload families are analysis errors, and the adapter rejects
otherwise valid numeric hardware before publication until its value-domain and
arithmetic lowering contracts are defined.

Numeric-vector validation (2026-09-22): all 83 focused VHDL tests passed—37
integrated constructor/NVC tests and all 46 lexer/parser/analyzer tests. The
standalone suite also passed from an isolated source copy.

## `numeric_std` vector comparisons

With `ieee.numeric_std.all` visible in the architecture, matching `unsigned`
or matching `signed` vectors resolve `=`, `/=`, `<`, `<=`, `>` and `>=` to a
`boolean` result. The operands may have different lengths and directions;
mixed signedness, null operands and missing visibility are deterministic
errors. Other relational overload families remain outside this increment.

This support does not change the adapter boundary: numeric vectors are analyzed
but are not published as SNL hardware.

Numeric-comparison validation (2026-09-22): all 85 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 48 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` vector multiplication

With `ieee.numeric_std.all` visible in the architecture, `*` resolves for two
matching `unsigned` vectors or two matching `signed` vectors. The result range
is `(L'length + R'length - 1) downto 0`, including when operand lengths, bounds
or directions differ.

Mixed signedness, null operands, missing visibility and unrepresentable result
widths are errors. Vector-scalar multiplication remains outside this increment
and is added by the later scalar/vector multiplication milestone; valid products
are not yet authorized for SNL publication.

Numeric-multiplication validation (2026-09-22): all 87 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 50 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` vector division family

With `ieee.numeric_std.all` visible in the architecture, `/`, `rem` and `mod`
resolve for two matching `unsigned` vectors or two matching `signed` vectors.
Division returns a canonical vector with the dividend length; remainder and
modulo return one with the divisor length.

Mixed signedness, null operands, missing visibility and unrepresentable result
widths are errors. A zero divisor is a value-level runtime error and is not
invented by type analysis. Scalar overloads and SNL lowering remain outside
this increment.

Numeric-division validation (2026-09-22): all 89 focused VHDL tests passed—37
integrated constructor/NVC tests and all 52 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## `numeric_std` signed unary negation

With `ieee.numeric_std.all` visible in the architecture, unary `-` resolves for
a non-null `signed` vector. The result retains the operand length and uses the
canonical range `length - 1 downto 0`. Unsigned, invisible, null and
unrepresentably wide operands are deterministic errors.

This semantic support does not authorize numeric-array SNL publication.

Signed-negation validation (2026-09-22): all 91 focused VHDL tests passed—37
integrated constructor/NVC tests and all 54 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## `numeric_std` scalar/vector addition and subtraction

`natural` is represented as an integer subtype rather than being collapsed
into unrestricted `integer`. With `ieee.numeric_std.all` visible, vector/scalar
and scalar/vector `+` and `-` resolve for `unsigned` with `natural`, and for
`signed` with `integer` or `natural`. A nonnegative integer literal resolves
contextually to `natural` for an unsigned overload. The result retains the
vector operand length and uses canonical descending bounds.

An unrestricted integer object cannot satisfy a `natural` formal without
subtype proof. Missing visibility, incompatible scalars and null vectors are
errors, and numeric arrays remain outside SNL publication.

Scalar-add/subtract validation (2026-09-22): all 93 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 56 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` scalar/vector comparisons

With `ieee.numeric_std.all` visible, `=`, `/=`, `<`, `<=`, `>` and `>=`
resolve in both operand orders between `unsigned` and `natural`, or between
`signed` and `integer` (including its `natural` subtype). Results are
`boolean`, and nonnegative integer literals are resolved contextually as
`natural` where the unsigned overload requires it.

An unrestricted integer object does not satisfy an unsigned comparison formal.
Missing visibility, null vectors and incompatible scalar types are errors, and
numeric arrays remain outside SNL publication.

Scalar-comparison validation (2026-09-22): all 95 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 58 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` scalar/vector multiplication

With `ieee.numeric_std.all` visible, `*` resolves in both operand orders between
`unsigned` and `natural`, or between `signed` and `integer` (including
`natural`). The package converts the scalar to the vector operand's length, so
the result length is twice that vector length and its bounds are canonical
descending. Nonnegative integer literals resolve contextually as `natural` for
unsigned multiplication.

Unrestricted integer objects do not satisfy the unsigned overload. Missing
visibility, null vectors, incompatible scalar types and unrepresentable doubled
widths are errors. Numeric arrays remain outside SNL publication.

Scalar-multiplication validation (2026-09-22): all 97 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 60 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.
