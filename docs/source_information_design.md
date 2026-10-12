<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Unified source information

## Scope and verified baseline

SNLRTLInfos remains the shared metadata container for RTL and structural designs.
Its name is retained for compatibility; source access does not imply RTL.
SNLSourceLoc currently stores an interned path and one range. SV's
SNLSVConstructor::annotateSourceInfo writes it and retainAliasSourceLocation
copies it onto generated producers. Consequently the legacy slot cannot safely
be classified as a declaration in every case. VHDLConstructor retains source
units and diagnostic spans but does not populate shared per-object locations.
The original SNLVRLConstructor preserved attributes without normalizing Yosys
src or storing typed declarations; the gate-Verilog stage below now implements both. SNLCapnPRTLInfos serializes the single range and extra map;
Verilog export uses sv_src_* / naja_sv_src. Python exposes getSourceLoc and
get_source_range. SV already has live AST link machinery; this is not yet a
provider-neutral contract.

## Core contract (this change)

An SNLSourceReference contains `range: SNLSourceLoc`, `provider: NLName`,
`language: NLName`, and `representation: NLName`. Empty qualifier names mean
unknown, not inferred. Provider identifies who supplied the reference (for
example slang, naja-vhdl, yosys, user); language describes the referenced input
(for example systemverilog, vhdl, verilog); representation describes that input
(for example rtl or gate). These are extensible interned identifiers, not enums.
Neither a path extension nor a generic src attribute establishes language or
representation. References require no retained text, AST, or filesystem access.

Ranges use one-based lines and byte columns with inclusive endpoints, matching
the existing SV location conversion. Zero means an unknown coordinate; an empty
path means unknown input identity. Missing endpoints remain zero; do not invent
a length. Lines and columns use uint32 (maximum 4,294,967,295); adapters must
reject unrepresentable coordinates rather than wrap them. This widens the
original uint16 columns and requires native consumers to rebuild, while Python
return shapes stay unchanged. Legacy string setters retain their invalid-value
and saturation behavior, with saturation now at the uint32 maximum. Paths are opaque producer strings:
do not resolve symlinks, canonicalize, require existence, or infer language.
Relative paths are relative to the producer's input context; relocation/context
mapping is future work. Two differently spelled paths remain distinct.

SNLRTLInfos gains:

- setSourceDeclaration(reference), getSourceDeclaration(), clearSourceDeclaration().
- addSourceOrigin(reference), getSourceOrigins(), clearSourceOrigins().
- hasSourceReferences() for allocation-free viewer flags covering declarations
  and origins. The independent legacy location is excluded.
- clearSourceLoc() to clear the legacy slot independently.

Declaration is optional and refers to the object's occurrence in the loaded
input. An instance's declaration is its instantiation, whereas the SNLDesign
model's declaration is its definition. Generated logic without a textual
declaration has no declaration. Origins are an ordered vector of upstream
references; insertion deduplicates exact range plus all qualifiers, preserving
first occurrence. Declaration and origins never implicitly overwrite each other.
Queries return empty optional/vector when absent and never allocate storage.
The additional allocation is lazy, held by one pointer per existing metadata
container; objects without SNLRTLInfos incur no additional cost. cloneInfos
makes an independent deep copy of the new data.

The legacy setSourceLoc/getSourceLoc and sv_src_* fields are unchanged and
independent. get_source_range() retains exactly its previous behavior; it does
not choose an arbitrary origin or fall back to a model. Existing frontend data
therefore remains available there but is not automatically reclassified into
new fields. Frontend migration must assign roles explicitly and dual-write the
legacy slot where compatibility requires it.

Raw Python SNLDesign and SNLDesignObject expose hasSourceReferences(),
getSourceDeclaration(), and getSourceOrigins(). A returned reference is a tuple
`(range, provider, language, representation)`; range is
`(file, line, column, end_line, end_column)` and unknown qualifiers are empty strings.
High-level immutable SourceReference uses SourceRange and optional string qualifiers.
Net, Term and Instance expose get_source_declaration() and get_source_origins();
Instance also exposes get_model_source_declaration() so definition lookup is explicit.
Python source access is read-only. Native loaders populate references; Python
setters and input conversion helpers are outside the current naja-schematic scope.

The core supports in-memory metadata, cloning, and typed NajaIF persistence.
Schema 0.2.0 adds SourceRange (32-bit columns), SourceReference, an independent
full-width legacy slot, a declaration, and ordered origins. Existing ordinals
retain their wire types; readers prefer the full-width legacy slot and can decode
the old slot at payload level without inventing roles. Bus-term bits and instance
terminals now have sparse metadata records as well. The snapshot manifest revision
comes from constants owned by naja-if. The conservative exact schema/producer
checks remain: older snapshots must be regenerated. Cross-build compatibility
policy is deferred; this change does not remove the producer safety gate.

Verilog export does not synthesize source-reference carriers. Source access for
naja-schematic uses the in-memory query APIs; typed NajaIF preserves the metadata.
Existing raw attributes and legacy compact/verbose source export are unchanged.

The source submodule uses naja-if commit
`0d28430f84d61e9d5973f989bfff303325a7510c`, which includes the schema from
`73b99c9183bcd88435325cf8e957276677178aaf` and native Bazel support on main.
Default Bazel selects this checkout through .bazelrc, matching CMake;
`--config=registered` clears that override and selects the registry version.
Source CI initializes the checkout; BCR validation runs independently through
the manual registered workflow. Default Bazel also selects the naja-verilog source checkout, whose native
module now uses BCR Bison/Flex and explicit C++ rule loads. Slang remains
registry-backed with exact source revision checks. The registered naja-if version `0.0.0-20260723-099677d9` predates the new
schema, so current source changes are not yet ready for BCR distribution.
Registry publication and compatibility validation are separate follow-up work
and do not block source or package changes.
All delivery paths must still preserve equivalent Naja/najaeda source APIs,
frontend population, coordinate widths, metadata round trips and snapshot
compatibility for the same release and requested features. Registry lag never
permits silently dropping metadata or shipping build-dependent source semantics.

## Frontend population

SV now dual-writes the independent legacy slot. Module/model definitions, ports,
ordinary named value-symbol nets and source instantiations receive declarations.
Generated lowering uses origins (including inlined function-local nets), labelled
provider `slang`, language `systemverilog`, representation `rtl`. Alias collapse
unions transferred origins in insertion order without overwriting the producer's
declaration; aliases with declarations are retained. These are textual references
and do not depend on keep_ast_link or retained AST/source ownership. Frontend
range selection still follows slang's fully original spelling locations.

Custom Verilog source-reference carriers are outside the current naja-schematic
scope. There is no special import or export handling for them; arbitrary
attributes stay raw. Native loader capture and explicit Yosys `src` decoding
populate the references consumed by viewers. NajaIF preserves typed metadata.

## Remaining stages and acceptance criteria

1. Frontend role migration follow-up: align VHDL with the shared SV declaration/
   origin roles, retaining source-unit ownership across work/library contexts.
   Extend SV coverage for macros, generated constructs, repeated elaboration,
   unknown spans and multi-source lowering. Audit remaining special net-creation
   paths and frontend alias transfers. Do not relabel inlined/generated objects
   as declarations merely because an AST symbol was used for lowering.
2. Gate Verilog population is implemented (see the contract and validation below).
   Follow-up: add a preprocessor source map, richer full-body spans and source
   context/path resolution. General raw-attribute persistence in NajaIF remains
   independent work. Do not infer AST linkage from textual origins.
3. Persistence follow-up: the reviewed naja-if schema is published; align the
   remaining CMake and Bazel source dependencies under the revised build policy;
   naja-if now uses the shared checkout. Validate BCR dependencies independently
   when publishing registry versions. Custom Verilog source-reference
   carriers are outside the current scope.
4. Transformations: audit clone, flatten, merge, uniquify and optimization paths.
   Supported design, instance, scalar, bus and bus-bit clones now copy the new data. Many-to-one operations
   must union upstream origins deterministically, preserve the destination's
   declaration, and avoid assigning one arbitrary source declaration to a result.
   Cross-library/database ownership and deduplication need transformation tests.
5. Optional AST integration in a separate change: provider-neutral context/node
   identifiers, multiple links, and explicit resolved/unresolved/ambiguous states.
   Adapters own or safely share compilation contexts; no raw dangling pointers.
   Core SNL headers must not include slang or VHDL AST headers. Reuse/audit SV's
   existing live link lifecycle; add VHDL adapter with equivalent ownership rules.
   Snapshots preserve symbolic links only, never process pointers. Linking Yosys
   origins to separately loaded RTL is later work, never implicit in this API.


## Consumer integration status

SV model definitions and source instantiations now expose separate declarations,
while lowered gates expose origins. Typed NajaIF round trips work locally. New source-reference export to
Verilog is outside the current viewer-support scope. Gate-level declaration capture and explicitly selected Yosys `src` normalization
now provide file/range cross-probing where synthesis retained provenance. VHDL population is the remaining language-alignment stage. Loading
legacy payloads cannot invent new roles. A downstream capability flag can use
hasSourceReferences() without traversing or allocating tuples/lists. No AST
linkage is inferred by any of these accessors.

The raw camelCase term-role API retains pre-0.7.29 snake_case aliases for consumers.
Wheel configuration now detects bundled parser output: sdists with generated
sources use them, while Git checkouts use Flex/Bison unless explicitly overridden.

## Gate-Verilog population contract and validation

`SNLVRLConstructor::Config::yosysSourceAttributes_` and Python
`yosys_source_attributes` (raw loader and `VerilogConfig`) default to false.
Explicit selection establishes Yosys provenance for this load. No automatic
banner, filename or attribute-name heuristic is used. Declarations are populated
independently, with provider `naja-verilog`, language `verilog`, representation
unknown. The legacy location slot remains untouched. Model definitions use the
module keyword/name span; instances use the name span; named built-in gates use
the name span; anonymous gates use the type span. Port declarations and explicit
net declarations use their grammar spans. Shared declarations give each object
the shared span and attributes. Non-ANSI ports preserve their attributes and
spans through deferred interface creation. Port nets inherit port source data,
then an explicit wire declaration supplies their own declaration. Generated
constant nets and unknown model stubs have no declaration. Spans are partial
textual declarations, not full module/instance bodies.

The upstream parser exposes a range separately from its legacy diagnostic
location. Bison exclusive byte endpoints are converted to inclusive shared
endpoints. No source/AST retention is required. With preprocessing enabled,
declarations are omitted because the preprocessor supplies no original source
map; explicit Yosys origins remain usable.

Yosys string `src` entries split at `|`, then at the final colon. Paths remain
opaque, including Windows drive letters, backslashes, spaces and additional
colons. The suffix grammar is `line[.column][-endLine[.endColumn]]`, with unsigned
decimal uint32 coordinates. Missing coordinates and explicit zero stay unknown.
Empty/malformed/non-string entries, overflow and known reversed ranges warn and
are skipped independently; valid entries remain ordered and exact duplicates
collapse. No language, RTL representation or AST link is inferred. Raw attributes
are retained unchanged in the loaded netlist and through Verilog export.
Typed roles survive NajaIF round trips. General raw Verilog attributes
are not currently serialized by NajaIF (an existing independent limitation).

The repository's Yosys 0.47+135 tinyrocket benchmark loaded with explicit Yosys
selection: five designs, 26,636 instances, 32,165 nets and 457 terms. All designs,
instances and terms had declarations; 32,155 nets had declarations (ten generated
constant nets had none). Origins were present on five designs, 3,508 instances,
7,543 nets and all 457 terms. For example `_8829_` in CSRFile points to gate file
line 30102 and an upstream `cells_adders.v` range 18.17–22.14. Origins may refer to
technology/model sources as well as original RTL; the API does not mislabel these
as RTL. Gates lacking synthesis provenance still have their loaded declaration.
Cross-probing currently means exposing these file/range references, without
resolving source availability, relocating paths or binding a separately loaded AST.

Both CMake and default Bazel now select the edited parser checkout. Registered
mode clears both naja-if and naja-verilog local overrides. Native parser Bazel
rules use module-provided Bison/Flex/headers; no Naja-owned overlays are added.
The macOS Bazel attempt selects/analyzes these sources but compilation is blocked
by the toolchain rejecting the SDKSettings.json absolute dependency in
rules_cc_autoconf. No CMake/Bazel behavior parity or BCR readiness is claimed.
The next precise delivery stage is to fix that local toolchain and execute the
same frontend, raw Python and round-trip checks against Bazel outputs, then
publish the dependency changes on their upstream repositories before advancing
the corresponding Naja gitlinks. This session leaves all changes uncommitted.

Yosys origin paths decode Verilog string escapes (backslash, quote, newline,
tab and up to three octal digits); the raw attribute spelling stays unchanged.
Unsupported escapes, NUL and out-of-byte-range octal escapes are diagnosed and
skipped per entry. Windows paths should use doubled backslashes in the Verilog
string literal. The parser scanner accepts escaped quotes in attribute strings.

Validation completed locally: 153 frontend/exporter CTest checks, 67 parser
checks, 12 raw source/attribute integration tests, 35 NLDB loader tests, three
high-level integration tests and six dependency-configuration tests passed.
The live source-dependency checker and Sphinx (Alabaster fallback) passed.
REUSE reported no missing notices, but two pre-existing invalid expressions
inside SV fixture strings remain in SNLSVConstructorTestAlwaysComb.cpp and
SNLSVConstructorTestMemoryInference.cpp. The temporary parser harness uses Naja's
existing googletest checkout because the parser's own test submodule is absent.
