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
SNLVRLConstructor preserves attributes without normalizing Yosys src or storing
typed declarations. SNLCapnPRTLInfos serializes the single range and extra map;
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
a length. Keep the existing uint32 line / uint16 column widths for compatibility;
new adapters must diagnose unrepresentable coordinates rather than wrap them.
Legacy values retain their historical behavior. Paths are opaque producer strings:
do not resolve symlinks, canonicalize, require existence, or infer language.
Relative paths are relative to the producer's input context; relocation/context
mapping is future work. Two differently spelled paths remain distinct.

SNLRTLInfos gains:

- setSourceDeclaration(reference), getSourceDeclaration(), clearSourceDeclaration().
- addSourceOrigin(reference), getSourceOrigins(), clearSourceOrigins().
- hasSourceReferences() for persistence capability checks.

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

Raw Python SNLDesign and SNLDesignObject expose getSourceDeclaration,
setSourceDeclaration (None clears), getSourceOrigins, addSourceOrigin and
clearSourceOrigins. A reference is a tuple `(range, provider, language,
representation)`; range is `(file, line, column, end_line, end_column)` and
unknown qualifiers are empty strings. Invalid types, negative coordinates and
overflow are rejected before mutation. High-level immutable SourceReference
uses SourceRange and optional string qualifiers. Net, Term and Instance expose
get_source_declaration() and get_source_origins(); Instance also exposes
get_model_source_declaration() so definition lookup is explicit. Experts can
populate data through raw bindings; high-level editing wrappers can be added
when frontend workflows establish a need.

This stage supports in-memory metadata and cloning. NajaIF and metadata-enabled
Verilog export reject new references explicitly until the persistence contract
lands. No schema change or dependency pin is made here. Legacy-only exports
continue to work. Export may have begun before rejection; callers must discard
failed output. Metadata-disabled Verilog export intentionally omits metadata.

## Remaining stages and acceptance criteria

1. Frontend role migration: SV and VHDL must dual-write declarations for designs,
   terms, nets and instantiations, and origins for lowered operators, processes,
   generated instances and aliases. Preserve source-unit ownership across VHDL
   work/library contexts. Cover generated constructs, repeated elaboration,
   unknown spans, macro expansion and multi-source lowering with focused tests.
   Distinguish original spelling origins from declaration locations explicitly.
2. Gate Verilog: add declaration spans to naja-verilog parser callbacks (upstream
   dependency change if required). Decode Yosys src as ordered `|`-separated
   references, parsing numeric suffixes without breaking colon-containing paths.
   Preserve raw attributes, reject/diagnose malformed or overflowing spans without
   fabricating coordinates, retain valid entries, use provider yosys only when
   that dialect is established, and leave unknown language/representation empty.
   A synthesized gate has its gate-netlist declaration plus zero or more upstream
   origins and no AST link by default. Test multiple origins, duplicates, Windows
   paths, malformed entries and ordinary non-Yosys src attributes.
3. Persistence: add typed declaration and reference-list fields in thirdparty/
   naja-if with an owned schema version and compatibility policy, then synchronize
   the submodule/Bazel pin (naja-if ancestor rule) and run the sync checker. Define
   neutral versioned Verilog attributes with escaping and unknown-field behavior;
   preserve legacy attributes during migration. Remove export guards only once
   round trips cover all roles, qualifiers, multiple origins and missing values.
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
