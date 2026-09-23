# Independent VHDL frontend

This directory is the future standalone repository root. It contains the
VHDL-2008 semantic probes and a Python-standard-library runner, plus an initial
handwritten C++20 lexer and recursive-descent parser. The parser handles entity
ports and integer-valued entity generic interfaces, internal signal declarations,
concurrent and conditional signal
assignments, and multiple scheduled writes in a restricted positive-edge process. The initial analyzer binds architectures to entities and
resolves names in assignment values, conditions, clock guards and process-local
variables. It evaluates locally static integer generic defaults and generic maps,
then substitutes them into constrained-vector bounds; its restricted scheduler
resolves immediate assignments and identifies
straight-line retained variable state. It type-checks a deliberately narrow set
of scalar, constrained `bit_vector`, and imported `std_logic`/`std_logic_vector`
expressions. Per-design-unit `library` and `use` clauses are preserved, with the
IEEE `std_logic_1164` and `numeric_std` package names recognized; general VHDL
type analysis and elaboration are not implemented yet. Nothing
here imports, links or discovers Naja/SNL, or requires the parent build.

Naja currently has a deliberately narrow integration proof in
`src/nl/formats/vhdl`: scalar `bit` and constrained `bit_vector` expressions and concurrent conditional
assignments are lowered to shared canonical SNL gates and the mux primitive. One clocked process
with internal signals, retained scalar variables and distinct scheduled targets
uses the shared DFF builder, or the shared DFFE builder for an optional active-high
clock enable, and the shared DFFSR builder for active-high synchronous reset to
zero. The standard reset-priority `if rst ... elsif en ...` combination uses the
shared DFFSRE builder. The proof rejects
nine-valued `std_logic` ports and every unsupported shape before creating a
design. It does not provide general VHDL type analysis or elaboration.
Defaulted generics can size ports in a single design. In the supported one-level
structural profile, integer generic maps create separate child specializations so
different actual values cannot silently share a model with the wrong port bounds.
Constrained array type declarations in architecture declarative parts are retained
in the syntax tree. The additional indexed RTL profile described below supports
static indexing and loops. Generate statements and general process loops
remain outside this profile and are rejected rather than discarded. Other valid
but unsupported architecture declarative items are identified directly in parser
diagnostics instead of being misreported as a missing architecture ``begin``.

### Indexed RTL synthesis profile

The Naja adapter additionally elaborates constrained arrays, nested static
indices, `others` aggregates, and static `for` loops in a positive-edge process.
Indices and bounds can use defaulted integer generics and loop parameters.
Nested `if`/`elsif`/`else` statements preserve source assignment priority;
variables update immediately while signal reads observe the current state.
This supports vector LFSRs with a tap table, synchronous reset and enable.
Both ascending and descending ranges retain their declared indexing and map
array assignments by position. Unconditional signal assignments after the
clock guard require every source signal in the process sensitivity list.

This profile synthesizes `bit` and imported `std_logic` scalars and vectors
as two-state hardware. It accepts only binary literals, rejects multiple
drivers, and does not model IEEE nine-valued simulation. `numeric_std` and
`std_logic_unsigned` imports may be present, but their arithmetic overloads
are not implemented here. Dynamic indexing, retained variables read before
definite assignment, array ports, hierarchy, initialization, and asynchronous
processes remain unsupported. Elaboration is bounded to 65,536 bits per object
and 100,000 expanded statements.

The standalone analyzer and scalar scheduler diagnose syntax requiring this
profile; the adapter performs its own checked RTL elaboration and discards the
design on any failure. The original scalar integration path remains available.

The frontend uses handwritten lexing and will use recursive descent parsing.
Its language model and VHDL-specific elaboration services will live here. The
Naja adapter and shared SV/VHDL hardware construction will remain outside this
directory. In-tree hosting is temporary; extraction is a required milestone.

## Build the standalone C++ slice

```sh
cmake -S . -B /tmp/naja-vhdl-build -DCMAKE_BUILD_TYPE=Release \
  -DVHDL_GTEST_SOURCE_DIR=/path/to/googletest
cmake --build /tmp/naja-vhdl-build
ctest --test-dir /tmp/naja-vhdl-build --output-on-failure
```

Tests use GoogleTest. Provide an installed CMake package for GTest, or point
`VHDL_GTEST_SOURCE_DIR` at a GoogleTest checkout when configuring. The extracted
repository will pin GoogleTest as its own test-only dependency (the current Naja
checkout pins commit `063de7e9578f82b369302001269680b4b1553359`). The exported
CMake target is `vhdl::frontend`. Tests cover basic identifier case
handling, extended identifiers, literal/apostrophe distinction, compound symbols,
source locations, port groups and ranges, expression precedence, and malformed
input recovery. They do not imply broad language-conformance coverage.

## Run the reference probes

Use an installed GHDL **6.0.0**, then run from this directory:

```sh
python3 tests/semantic/run.py --ghdl /path/to/ghdl --report /tmp/vhdl-results.json
python3 tests/semantic/run_nvc.py --nvc /path/to/nvc --report /tmp/vhdl-nvc-results.json
python3 -m unittest discover -s tests/semantic -p test_runner.py
```

`tests/semantic/reference.json` records the exact tested distribution and source
package hashes. The runner checks the compiler version; it does not assume a
distribution layout or enforce those package hashes on other installations.
The asset hash pins the archive used for the baseline, not every GHDL 6.0.0 build.

The runner enforces the version, uses VHDL-2008 mode, and creates a fresh work
directory for every case. It records commands, exit statuses, diagnostics,
source fingerprints and compiler identity in the JSON report. Per-command
wall-clock limits supplement simulation time/delta limits. Successful cases
must reach their own completion marker; rejected cases must fail at an expected
stage with the expected diagnostic. A crash or timeout never counts as a pass.
Use repeated `--case NAME` arguments to select probes.

`tests/semantic/manifest.json` is the case inventory. The 35 probes exercise
identifiers/literals, overload resolution, array bounds/direction, numeric and
aggregate semantics, packages, hierarchy/binding, process scheduling, and
expected errors. `analysis_only` marks behavior outside the initial binary
hardware profile, such as resolved multi-driver logic and weak clock levels.
Testbench assertions, delays and termination are reference-test infrastructure,
not a proposed synthesizable language subset. `rtl` labels an intended semantic
area, not implemented frontend support.

All cases are locally authored under Apache-2.0. Standard packages come from the
selected reference installation; they are not copied into this tree. These tests
establish a reference baseline, not standards conformance or Naja VHDL support.
A second independent runtime comparison is recorded in
`tests/semantic/reference_nvc_1.23.0.json`. It covers the same 35 probes; the
report records three diagnostic or detection-stage differences from the GHDL
baseline, so those should not be mistaken for frontend behavior. The NVC runner
pins version 1.23.0 and records its version-specific diagnostic and stage
overrides in `tests/semantic/nvc_expectations.json`.

To check independence, copy this directory alone to a temporary location and run
the same commands there. The standalone CMake library and exported target can also be built, installed
and consumed without any Naja/SNL dependency.

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
registers, active-low controls,
unassigned retained variables, repeated targets, multiple drivers, undriven internal
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

The standalone analyzer now records scalar expression types for `bit`, `boolean`,
`integer`, `real` and `string` declarations and literals. It checks assignment
compatibility, boolean conditional guards, equality operands, `not`, and matching
`bit` or `boolean` operands for `and`, `nand`, `or`, `nor`, `xor` and `xnor`.
Unsupported operators, types and ambiguous character literals produce diagnostics
instead of authorizing hardware construction.

The Naja adapter lowers nested scalar `bit` names, `'0'`/`'1'` literals and those
logical operators through `SNLRTLPrimitives::createGate()`. Conditional branches
may contain the same expressions and still use the shared mux builder; the narrow
condition profile remains `name = '1'`. Canonical gate models are shared with the
SystemVerilog frontend. Boolean hardware, arithmetic, general equality lowering,
logical expressions in clocked assignments, vector selection and conversions remain
unsupported and are rejected before design publication.

Scalar-expression validation (2026-09-21): all 52 focused lexer, parser,
analyzer, shared-primitive, adapter, SystemVerilog and NVC reference tests
passed. All 26 standalone tests passed from an isolated copy; installation and
a separate client using `ScalarType` through only `vhdl::frontend` also passed.

## Constrained vector proof

The analyzer recognizes one-dimensional constrained `bit_vector` declarations,
preserves left/right bounds and direction, and accepts logical, conditional and
assignment operands when their lengths match. Direction and index values may
differ because VHDL array assignment is positional; mismatched lengths,
unconstrained or null arrays and non-binary vector types are rejected.

The adapter creates SNL buses with the source bounds intact. Shared bitwise gate
lowering maps each rightmost source element to hardware position zero, for both
`to` and `downto`, while the vector mux consumes the same least-significant-bit-
first boundary. This slice supports concurrent vector expressions only; vector
clocked state, indexing, slicing, concatenation, aggregates and string-literal
hardware remain future work.

Vector validation (2026-09-21): all 59 focused frontend, primitive, adapter,
SystemVerilog and NVC reference tests passed. All 27 standalone tests passed
from an isolated copy, followed by installation and a separate client reading
the preserved vector range through only `vhdl::frontend`.

## One-level direct-entity hierarchy proof

The standalone parser and analyzer now retain and bind labeled
`entity work.<name> port map (...)` statements with positional name-only
actuals. They diagnose duplicate labels, missing entities or actuals, wrong
arity, unsupported libraries, and scalar/vector width mismatches without any
Naja dependency.

The Naja adapter overload `construct(source, top)` requires an explicit top and
lowers one structural level: existing behavioral lowering builds each leaf,
then the adapter creates the top instances and connects complete terms and nets
by position. Supported interfaces and internal signals are `bit` or constrained
non-null `bit_vector` with `in`/`out` ports. Named and `open` associations,
components/configurations, nested hierarchy, ambiguous architectures, mixed
behavior in the top, and missing or multiple drivers are rejected before any
design is published. A two-inverter vector fixture exercises different bounds
and directions and is compared with NVC.

Hierarchy validation (2026-09-21): all 60 VHDL lexer, parser, analyzer,
adapter and NVC reference tests passed in the integrated build. All 31
standalone frontend tests passed from a separate build tree.

## Standard rising-edge clock proof

Clocked processes now accept `if rising_edge(clk) then` in addition to the
explicit event-and-level idiom. The AST records which spelling was used while
name analysis and lowering share the same positive-edge clock semantics. Calls
with missing, multiple, literal or compound arguments remain parser errors.
The pipeline fixture now uses `rising_edge`, so its multi-register scheduling
and cycle behavior are checked against NVC; explicit `'event` tests remain in
place as regressions. The synchronous reset profile is covered below;
asynchronous reset, falling-edge, active-low control and general function-call
semantics remain unsupported.

Rising-edge validation (2026-09-21): all 63 integrated VHDL tests passed,
including the NVC pipeline comparison. All 33 standalone frontend tests passed
from a separate build tree.

## Active-high clock-enable proof

A clocked process may wrap all scheduled writes in exactly one nested
`if en = '1' then ... end if;`. The AST retains the enable name and literal,
the analyzer requires a scalar `bit`, and the adapter requires an input port and
the active-high literal `'1'`. Enabled writes use the shared
`SNLRTLPrimitives::createDFFE()` helper and the canonical NLDB0 DFFE C/D/E/Q
pins; a false enable therefore holds every scheduled state element, including
retained process variables.

The `enabled.vhd` NVC fixture checks enabled update, disabled hold and re-enabled
update, producing `1, 1, 0`. Active-low enables, deeper nested control and other
enable expressions are rejected before design publication; reset-priority
combination is accepted only by the profile below.

Clock-enable validation (2026-09-22): all 68 integrated VHDL tests passed,
including the NVC comparison; all 36 standalone frontend tests passed. The
shared primitive and SystemVerilog regression passed all 1,119 tests.

## Active-high synchronous reset proof

A positive-edge process may use exactly one nested
`if rst = '1' then ... else ... end if;`. The reset branch must assign `'0'` to
exactly the same scalar signal targets written by the data branch. The analyzer
binds and type-checks both branches; the adapter requires `rst` to be an input
port and rejects the complete process before publication when the branch shapes
do not match.

Each state element lowers through `SNLRTLPrimitives::createDFFSR()` to the
canonical NLDB0 DFFSR C/D/R/Q pins. The `reset.vhd` NVC fixture proves that
asserting reset between edges does not change Q, and that reset takes effect on
the next rising edge; its trace is `1, 1, 0, 1`. Multiple scheduled targets use
the same reset net. Active-low, asynchronous, reset-to-one and variable-state
forms remain unsupported.

Synchronous-reset validation (2026-09-22): all 72 integrated VHDL tests passed,
including the NVC comparison; all 38 standalone frontend tests passed. The
shared primitive and SystemVerilog regression passed all 1,120 tests.

## Synchronous reset with active-high clock enable

The combined profile accepts exactly
`if rst = '1' then ... elsif en = '1' then ... end if;` inside a supported
positive-edge process. Reset has priority over enable, both controls must be
scalar input `bit` ports, and the reset branch follows the same target-set and
reset-to-zero rules as the reset-only profile.

Each state element lowers through `SNLRTLPrimitives::createDFFSRE()` to the
canonical NLDB0 DFFSRE C/D/E/R/Q pins. The `reset_enable.vhd` NVC fixture checks
enabled update, disabled hold, synchronous reset, reset-over-enable priority and
re-enabled update; its trace is `1, 1, 1, 0, 0, 1`. Active-low controls, an
additional else branch, deeper control flow and process-variable reset remain
explicit rejection boundaries.

Combined reset-enable validation (2026-09-22): all 81 focused VHDL/frontend and
shared-primitive tests passed, including all nine NVC comparisons. The broad
shared primitive and SystemVerilog regression passed all 1,137 tests.

## Library context and nine-valued analysis foundation

The parser now preserves `library` and `use` context separately for every entity
and architecture design unit, including comma-separated library names and
selected package names. Analysis recognizes whole-package imports of
`ieee.std_logic_1164.all` and `ieee.numeric_std.all`, diagnoses a missing library
clause, partial imports and unsupported packages, and does not leak visibility
from one design unit to the next.

An imported `std_logic` or constrained `std_logic_vector` has a distinct analyzer
type rather than being treated as `bit`. Logical and equality expressions retain
that type, and all nine `std_logic` character values are recognized in typed
contexts. The Naja adapter intentionally continues to reject these ports before
design publication because resolved drivers, unknown values and other non-binary
behavior have no faithful SNL representation yet. `numeric_std` visibility is
recorded, but its types and operators remain a later slice.

Context/type validation (2026-09-22): all 76 VHDL tests passed—36 integrated
constructor/NVC tests and 40 standalone parser/analyzer tests. The previously run
1,137-test shared primitive/SystemVerilog regression remains unchanged because
this slice touches only the VHDL syntax and semantic layers.

## `numeric_std` vector arithmetic analysis foundation

The analyzer resolves constrained `unsigned` and `signed` when
`ieee.numeric_std.all` is visible in the current design unit. Matching
vector-vector operands support `+` and `-`; the recorded result range is
`max(left'length, right'length) - 1 downto 0`, matching the package overloads.
Entity context does not make an overload visible in a following architecture.

Mixed signedness, missing imports, unconstrained or null numeric arrays and
other overload families remain explicit diagnostics. This standalone layer only
establishes semantic identity and result shape; the Naja adapter still rejects
numeric arrays rather than collapsing their `std_logic` element domain into
binary hardware.

Numeric-vector validation (2026-09-22): all 83 focused VHDL tests passed—37
integrated constructor/NVC tests and all 46 lexer/parser/analyzer tests. The
standalone suite also passed from an isolated source copy.

## `numeric_std` vector comparisons

Matching `unsigned` or matching `signed` vectors support `=`, `/=`, `<`, `<=`,
`>` and `>=` when `ieee.numeric_std.all` is visible in the architecture. The
operands may differ in length or direction and the result is `boolean`.
Mixed signedness, null operands and invisible operators are diagnosed. The
scalar/vector relational overloads are described in their later section below.

This is standalone semantic support only. Numeric vectors continue to be
rejected by the Naja adapter before SNL publication.

Numeric-comparison validation (2026-09-22): all 85 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 48 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` vector multiplication

Matching `unsigned` or matching `signed` vectors support `*` when
`ieee.numeric_std.all` is visible in the architecture. The result has the
canonical range `(L'length + R'length - 1) downto 0`, regardless of operand
bounds or directions.

Mixed signedness, null operands, invisible overloads and unrepresentable result
widths are diagnosed. Scalar/vector multiplication is described in its later
section below; division-family scalar overloads remain future work. The Naja
adapter continues to reject numeric-array hardware.

Numeric-multiplication validation (2026-09-22): all 87 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 50 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` vector division family

Matching `unsigned` or matching `signed` vectors support `/`, `rem` and `mod`
when `ieee.numeric_std.all` is visible in the architecture. Division returns
the dividend length; remainder and modulo return the divisor length. Results
use canonical descending bounds.

Mixed signedness, null operands, invisible overloads and unrepresentable result
widths are diagnosed. Zero-divisor behavior remains a runtime package concern.
Scalar/vector overloads are described in their later section below; SNL
lowering remains future work.

Numeric-division validation (2026-09-22): all 89 focused VHDL tests passed—37
integrated constructor/NVC tests and all 52 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## `numeric_std` signed unary negation

A non-null `signed` vector supports unary `-` when `ieee.numeric_std.all` is
visible in the architecture. The result keeps the operand length and uses
canonical descending bounds. Unsigned, invisible, null and unrepresentably
wide operands are diagnosed.

The Naja adapter continues to reject numeric-array hardware before SNL
publication.

Signed-negation validation (2026-09-22): all 91 focused VHDL tests passed—37
integrated constructor/NVC tests and all 54 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## `numeric_std` scalar/vector addition and subtraction

The analyzer distinguishes `natural` from unrestricted `integer` and resolves
nonnegative integer literals contextually. With `ieee.numeric_std.all` visible,
vector/scalar and scalar/vector `+` and `-` support `unsigned` with `natural`,
and `signed` with `integer` or `natural`. Results retain the vector length with
canonical descending bounds.

An arbitrary integer object does not satisfy an unsigned overload requiring
`natural`. Missing visibility, null vectors and incompatible scalar operands
are diagnosed. Numeric-array SNL publication remains disabled.

Scalar-add/subtract validation (2026-09-22): all 93 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 56 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` scalar/vector comparisons

The six comparison operators resolve in either operand order between
`unsigned` and `natural`, or between `signed` and `integer` (including
`natural`), when `ieee.numeric_std.all` is visible. Results are `boolean`, and
nonnegative integer literals resolve contextually as `natural` for unsigned
comparisons.

Unrestricted integer objects cannot satisfy unsigned comparison overloads.
Missing visibility, null vectors and incompatible scalar types are diagnosed.
Numeric-array SNL publication remains disabled.

Scalar-comparison validation (2026-09-22): all 95 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 58 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` scalar/vector multiplication

Multiplication resolves in either operand order between `unsigned` and
`natural`, or between `signed` and `integer` (including `natural`), when
`ieee.numeric_std.all` is visible. The scalar is converted to the vector
operand's length, so the result has twice that vector length with canonical
descending bounds. Nonnegative integer literals resolve contextually as
`natural` for unsigned multiplication.

Unrestricted integer objects cannot satisfy unsigned multiplication overloads.
Missing visibility, null vectors, incompatible scalar types and unrepresentable
doubled widths are diagnosed. Numeric-array SNL publication remains disabled.

Scalar-multiplication validation (2026-09-22): all 97 focused VHDL tests
passed—37 integrated constructor/NVC tests and all 60 standalone
lexer/parser/analyzer tests. The standalone suite also passed from an isolated
source copy.

## `numeric_std` scalar/vector division family

The `/`, `rem` and `mod` operators resolve in either operand order between
`unsigned` and `natural`, or between `signed` and `integer` (including
`natural`), when `ieee.numeric_std.all` is visible. Each result retains the
numeric vector operand's length with canonical descending bounds. Nonnegative
integer literals resolve contextually as `natural` for unsigned operations.

Unrestricted integer objects cannot satisfy unsigned overloads. Missing
visibility, null vectors and incompatible scalar types are diagnosed. A literal
zero divisor remains type-correct because divide-by-zero is a runtime package
concern. Numeric-array SNL publication remains disabled.

Scalar-division validation (2026-09-22): all 99 focused VHDL tests passed—37
integrated constructor/NVC tests and all 62 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## `numeric_std` signed absolute value

A non-null `signed` vector supports unary `abs` when `ieee.numeric_std.all` is
visible. The result keeps the operand length and uses canonical descending
bounds, including for ascending and nonzero-based operands. Unsigned, invisible,
null and unrepresentably wide operands are diagnosed.

Predefined scalar absolute value is outside this package-specific increment, and
numeric-array SNL publication remains disabled.

Signed-absolute validation (2026-09-22): all 101 focused VHDL tests passed—37
integrated constructor/NVC tests and all 64 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## `numeric_std` vector logical operators

Unary `not` and binary `and`, `or`, `nand`, `nor`, `xor` and `xnor` resolve for
`unsigned` and `signed` vectors when `ieee.numeric_std.all` is visible. Binary
operands must have matching numeric types and lengths; source directions and
bounds may differ. Results preserve the operand length with canonical descending
bounds.

Mixed signedness, unequal or null vectors, and missing visibility are diagnosed.
Scalar `std_ulogic` broadcasts remain outside this increment, and numeric-array
SNL publication remains disabled.

Numeric-logical validation (2026-09-22): all 103 focused VHDL tests passed—37
integrated constructor/NVC tests and all 66 standalone lexer/parser/analyzer
tests. The standalone suite also passed from an isolated source copy.

## Naja Python loading API

The raw ``najaeda.naja.NLDB.loadVHDL(file, top=None)`` binding and the
high-level ``najaeda.netlist.load_vhdl(file, top=None)`` helper load one source
file through the same experimental adapter. The optional top name is required
for the supported multi-unit structural hierarchy. The high-level helper accepts
path-like objects, validates arguments and the input path before native loading,
and returns the usual top ``Instance`` wrapper.
