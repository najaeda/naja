# Python Native Interoperability Roadmap

- Status: active
- Primary Naja branch: `dev/python-native-interop`
- Kepler Formal implementation branch: create `dev/python-native-interop` in
  the Kepler Formal repository when Milestone 4 starts
- Last updated: 2026-09-10

## Goal

Provide one Python process in which an application or AI agent can safely use:

- the raw `najaeda.naja` API for elaborated SNL netlist objects;
- bundled pyslang bindings for the retained SystemVerilog syntax and semantic
  model; and
- a future `pykepler_formal` API for formal analysis of those same SNL objects.

The implementation must not exchange C++ pointers between independently built
copies of slang or Naja. Incompatible native modules must fail closed with a
Python exception before dereferencing an object.

There is no preliminary coexistence-only phase. The first supported pyslang
integration must use a unified native runtime and return real bundled pyslang
objects.

## Target architecture

SNL is the interoperability hub. Kepler Formal does not need a direct bridge to
pyslang:

```text
                         +-------------------------+
                         | private shared slang    |
                         | runtime                 |
                         +------------+------------+
                                      ^
                                      |
                 +--------------------+--------------------+
                 |                                         |
        najaeda.pyslang                           Naja SV frontend
                                                          |
                                                          v
                                              +-----------------------+
                                              | shared Naja runtime   |
                                              | SNL / DNL / wrappers  |
                                              +-----------+-----------+
                                                          ^
                                             +------------+------------+
                                             |                         |
                                      najaeda.naja           pykepler_formal
```

The expected Python composition is:

```python
from najaeda import naja
from najaeda import pyslang
import pykepler_formal as formal

top0 = db0.loadSystemVerilog(files0, keep_ast_link=True)
top1 = db1.loadSystemVerilog(files1, keep_ast_link=True)

frontend0 = naja.frontend_session(db0)
proof = formal.sec(top0, top1)

for mismatch in proof.mismatches:
    snl_object = mismatch.object
    ast_symbol = frontend0.symbol_of(snl_object)
```

Names in this example are provisional until their API review milestone.

## Architectural decisions

1. **One slang runtime per supported interop domain.** `najaeda.naja` and
   `najaeda.pyslang` must link to the same private shared slang library built
   from Naja's pinned slang checkout.
2. **One Naja runtime per supported interop domain.** `pykepler_formal` must
   reuse the Naja shared libraries loaded by `najaeda`; it must not vendor or
   statically embed another Naja copy.
3. **Naja is the object hub.** Pyslang symbols map to SNL objects through the
   retained frontend session. Kepler accepts and returns SNL objects. No direct
   Kepler-to-pyslang pointer bridge is planned.
4. **Version metadata is diagnostic, not proof of compatibility.** Compatibility
   requires a versioned native API and a process-local runtime identity token,
   not only matching Python package versions or Git hashes.
5. **Native crossings use private, versioned C API capsules.** User code receives
   typed Python objects; raw pointer capsules remain an implementation detail.
6. **Fail closed.** A foreign top-level `pyslang` object or an object from a
   different Naja runtime produces a dedicated Python exception before any
   native pointer is used.
7. **Formal analysis is non-destructive by default.** It may rebuild ephemeral
   DNL state under a runtime lock, but it must restore global Naja context and
   must not destroy input SNL databases or libraries.
8. **Do not expose ephemeral DNL identity.** Python-facing formal results use
   SNL objects or stable SNL identifiers/paths, never a `DNLID` that becomes
   invalid when DNL is rebuilt.
9. **Keep Kepler packaging separate.** The `pykepler-formal` distribution remains
   separate from `najaeda` and consumes its native runtime. This also keeps the
   projects' release and licensing responsibilities explicit.

## Native compatibility contracts

### Slang bridge

The bundled pyslang extension should expose a private API capsule similar to:

```c
typedef struct {
  uint32_t api_version;
  const char* slang_build_id;
  const void* runtime_identity;
  PyObject* (*wrap_compilation)(void* compilation, PyObject* owner);
  PyObject* (*wrap_symbol)(void* symbol, PyObject* owner);
  void* (*unwrap_symbol)(PyObject* symbol);
} NajaPyslangAPI_v1;
```

The owner argument must keep the retained frontend state alive for as long as a
borrowed pyslang view exists.

### Naja consumer bridge

`najaeda.naja` should expose a private API capsule for native consumers:

```c
typedef struct {
  uint32_t api_version;
  const char* naja_build_id;
  const void* runtime_identity;
  void* (*unwrap_design)(PyObject* design);
  void* (*unwrap_object)(PyObject* object);
  PyObject* (*wrap_object)(void* object);
} NajaPythonAPI_v1;
```

`pykepler_formal` must import this table, validate it, and use Naja's wrapper
functions rather than creating a second Python representation of SNL classes.
The real declarations should use opaque C-compatible types at the capsule
boundary and typed casts only after validation inside a compatible build.

## Milestones

### Milestone 0 - Freeze the contracts and build a failure prototype

Status: complete

- [x] Write an ADR for native ownership, runtime identity, versioning, and object
  lifetime.
- [x] Define stable names and versions for both private C API capsules.
- [x] Define `slang_build_info()` and `naja_build_info()` result schemas.
- [x] Prototype the runtime-identity check and prove that a deliberately foreign
  module is rejected without dereferencing its pointer.
- [x] Decide how bundled pyslang registrations avoid collisions with an external
  top-level pyslang installation; evaluate module-local pybind11 registration.

Exit criteria:

- Contract headers compile as C and C++.
- Compatible and incompatible-runtime unit tests pass.
- No public API exposes an unchecked pointer capsule.

### Milestone 1 - Build bundled pyslang against one slang runtime

Status: complete

- [x] Add a Naja wheel build option for bundled pyslang.
- [x] Build slang once as a private shared library for the Python wheel.
- [x] Give the library a Naja-private name to avoid system-library collisions.
- [x] Build pyslang from the exact `thirdparty/slang` checkout used by the Naja
  SystemVerilog frontend.
- [x] Package the extension under the `najaeda` namespace rather than claiming the
  external top-level `pyslang` package.
- [x] Pin the pybind11 build requirement used by the bundled bindings.
- [x] Add reproducible stub generation and packaging.
- [x] Preserve the existing standalone CMake build unless the interop option is
  enabled.

Exit criteria:

- `from najaeda import naja, pyslang` succeeds through the package's
  provider-first initialization, and external/bundled pyslang imports succeed
  in both orders.
- Native dependency inspection shows one slang runtime for both extensions.
- The bundled pyslang smoke tests and existing najaeda tests pass.
- Wheel repair does not duplicate or rename slang into separate per-extension
  copies.

### Milestone 2 - Return real pyslang objects from retained frontend state

Status: planned

- Replace the "latest compilation" interop model with an NLDB-specific frontend
  session.
- Convert retained `Compilation*` and `Symbol*` pointers into non-owning bundled
  pyslang wrappers through the versioned bridge.
- Convert bundled pyslang symbols back to SNL objects.
- Preserve current curated intent helpers for users who want plain Python data.
- Define destruction behavior for an NLDB with live frontend views.
- Support multiple simultaneously loaded SystemVerilog databases without
  cross-association.

Exit criteria:

- Compilation and symbol results are genuine `najaeda.pyslang` objects.
- SNL -> symbol -> SNL round trips preserve object identity.
- Foreign external-pyslang objects raise a dedicated compatibility exception.
- Destroyed database, multiple-database, and wrapper-lifetime tests pass under
  sanitizers where available.

### Milestone 3 - Publish Naja as a native runtime provider

Status: in progress

- [x] Add the versioned Naja consumer C API capsule.
- Establish a stable wheel location for Naja shared libraries and headers or a
  small build SDK.
- Provide a supported discovery mechanism for consumer builds, for example a
  CMake package directory reported by `python -m najaeda.cmake`.
- Define the Naja native ABI/build ID and compatibility policy.
- [x] Provide a process-local Naja runtime identity token.
- Document how auditwheel, delocate, and delvewheel consumers must leave the
  provider's Naja libraries unbundled.

Exit criteria:

- A minimal external test extension accepts an `SNLDesign`, calls a native Naja
  query, and returns the same Naja Python object.
- The extension fails cleanly with an intentionally incompatible runtime.
- Import order does not affect the selected runtime.

### Milestone 4 - Create the `pykepler_formal` extension

Status: planned

Repository: `kepler-formal`

- Create the matching development branch in the Kepler Formal repository.
- Split its build modes:
  - standalone CLI: retain the self-contained vendored Naja build;
  - Python extension: find and reuse the Naja runtime supplied by `najaeda`.
- Do not copy or package `naja.so`, `libnaja_*`, or slang in the Python wheel.
- Import and validate `najaeda.naja._C_API` during extension initialization.
- Initially bind focused formal entry points accepting raw `SNLDesign` objects.
- Return proof results containing Naja-wrapped SNL objects and Kepler-owned
  immutable result data.

Exit criteria:

- One script can load with najaeda, inspect with bundled pyslang, run formal
  analysis, and map a result back to its source symbol.
- Native dependency inspection proves that pykepler and najaeda resolve the same
  Naja runtime.
- An incompatible najaeda build causes `ImportError` or
  `IncompatibleNajaRuntimeError`, not a crash.

### Milestone 5 - Make Kepler lifecycle safe for an interactive Python process

Status: planned

- Introduce a shared Naja analysis/runtime lock.
- Audit all Kepler changes to `NLUniverse` top design and DNL lifetime.
- Use exception-safe RAII to restore the previous top and discard only
  Kepler-created ephemeral DNL state.
- Keep strong Python references to every input database/design while native
  analysis uses it.
- Release the GIL around long formal solving only after acquiring the native
  lifetime/concurrency guard.
- Disable compact/destructive behavior by default; expose it only through a
  clearly consuming API if it remains necessary.
- Convert all result references away from ephemeral DNL IDs before returning to
  Python.

Exit criteria:

- Exceptions and cancellation restore the previous Naja context.
- Inputs remain usable through najaeda after a default formal run.
- Concurrent edits or analyses are serialized or rejected deterministically.
- No returned object depends on destroyed DNL storage.

### Milestone 6 - Cross-platform wheels and compatibility matrix

Status: planned

- Test supported CPython versions on Linux x86-64/aarch64, macOS arm64, and
  Windows x86-64, including supported free-threaded builds.
- Test all import orders for `najaeda.naja`, bundled pyslang, and
  `pykepler_formal`.
- Test the presence of an external top-level pyslang installation and verify
  that foreign-object crossing fails closed.
- Inspect `ldd`/`readelf`, `otool`, and Windows dependency metadata in CI.
- Assert exactly one Naja runtime and one bundled slang runtime in the supported
  interop domain.
- Run ASan/UBSan builds for pointer provenance and destruction tests where the
  platform supports them.

Exit criteria:

- Repaired wheels pass clean-environment installation and end-to-end tests.
- CI detects accidental vendoring or duplicate-runtime regressions.

### Milestone 7 - Documentation and release contract

Status: planned

- Document the raw interoperability API in najaeda's `raw_api.rst`.
- Document ownership, lifetime, concurrency, and destructive-operation rules.
- Publish a supported Naja/pykepler compatibility table.
- Document the distinction between external top-level pyslang and bundled
  `najaeda.pyslang`.
- Add an agent-oriented example that traverses source AST, SNL, and a formal
  result in one script.
- Review binary redistribution and license notices for the separate wheels.

Exit criteria:

- Sphinx documentation builds successfully.
- Release checks validate the declared native compatibility metadata.

## Cross-repository rules

- The Naja repository is authoritative for the Naja Python C API, native build
  ID, runtime discovery, and SNL wrapper identity.
- The pinned slang checkout in Naja is authoritative for bundled pyslang.
- Kepler Formal's standalone build may continue pinning Naja independently, but
  its Python extension must build against the supported Naja runtime provider.
- Changes to capsule contracts land in Naja before their Kepler consumer.
- Each repository keeps focused unit tests; the final end-to-end compatibility
  test runs from the `pykepler-formal` wheel workflow.

## Risks and decision checkpoints

- **Shared-library wheel repair:** verify experimentally before committing to
  the final layout. Repair tools must not create separate slang or Naja copies.
- **Pyslang registry isolation:** the bundled target currently uses a private
  pybind11 namespace and internals key because `module_local` does not cover
  native enums. Keep the real external-pyslang two-order test in CI so a future
  pybind11 or slang update cannot silently invalidate the isolation mechanism.
- **C++ ABI stability:** until a stable Naja ABI is deliberately maintained,
  pykepler wheels should require an exact supported Naja build and still verify
  runtime identity at import.
- **Frontend lifetime:** the retained AST is currently owned by the NLDB.
  Milestone 2 must choose between preventing destruction while views exist and
  extending AST ownership independently while invalidating SNL back-links.
- **Global DNL state:** Kepler frequently destroys and rebuilds DNL. The Python
  design must serialize these operations and never expose their ephemeral IDs.
- **Free-threaded Python:** support requires an explicit audit; a wheel tag alone
  does not establish safety.

## Progress log

### 2026-09-10

- Created the persistent goal and Naja branch `dev/python-native-interop`.
- Confirmed that Naja already retains compilation, syntax trees, and
  bidirectional SNL/symbol associations in `SNLSVLiveASTLink`.
- Confirmed that current Naja and official pyslang builds embed separate static
  slang copies, so a Python version pin cannot make pointer exchange safe.
- Confirmed that standalone Kepler Formal links Naja shared libraries and copies
  `naja.so` for its embedded Python environment.
- Confirmed that Kepler changes the process-global Naja top and rebuilds DNL;
  compact miter execution can destroy input netlist storage.
- Saved the initial implementation roadmap.
- Started Milestone 0.
- Accepted ADR 0001: one Naja runtime, one Naja-owned slang runtime, strict
  build and runtime identity validation, canonical najaeda wrappers, and an
  isolated pybind11 domain for bundled pyslang registrations.
- Added version 1 C-layout contracts for the Naja consumer and bundled pyslang
  bridges, plus C and C++ compatibility/failure tests.
- Added the process-local runtime token to the shared Naja netlist library and
  exposed the private `najaeda.naja._C_API` provider table.
- Added `naja_build_info()` without exposing the runtime address to Python.
- Added a separately compiled test consumer that validates the provider,
  round-trips canonical `SNLDesign` and design-object wrappers, rejects wrong
  Python types, and works when imported before the provider.
- Started Milestone 1 with a `BUILD_NAJA_PYSLANG` wheel option and pinned
  pybind11 3.0.4 build dependency.
- Built `najaeda.pyslang` from the pinned slang checkout and linked it and the
  Naja frontend to one private `libnaja_slang` shared runtime.
- Added the bundled slang build-info function, runtime identity token, and
  private C API provider capsule.
- Isolated bundled pybind11 class and native-enum registries from external
  pyslang without modifying the slang submodule.
- Pinned ``pybind11-stubgen`` and added install-time PEP 561 stubs generated
  from the installed bundled extension; normalized the private capsule address
  so repeated generation is byte-identical.
- Moved the bundled-pyslang adapter, runtime shim, and packaging helpers into
  `thirdparty/pyslang`; retained the versioned capsule contract with Naja's
  first-party Python API headers.

Verified on 2026-09-10:

- CMake built the raw `naja` extension and test consumer.
- CTest native C/C++ contract tests and the complete raw Python test suite
  passed.
- Bazel native C/C++ contract tests passed and the full `naja.so` target built.
- macOS dependency inspection showed that both provider and consumer resolve
  `@rpath/libnaja_nl.dylib`; the consumer does not embed Naja.
- Sphinx HTML documentation built successfully with the existing raw-extension
  autodoc signature warnings.
- A fresh macOS Python build imported Naja and bundled pyslang, parsed and
  elaborated a real module, and validated the private pyslang capsule.
- Dependency inspection showed both native extensions resolving the same
  private `libnaja_slang` image.
- Real PyPI `pyslang==11.0.0` and `najaeda.pyslang` created distinct live slang
  object types and coexisted in both import orders.
- A clean PEP 517 build and delocate repair produced a wheel containing one
  physical unversioned `libnaja_slang.dylib`; both extensions resolve that
  exact file through `@loader_path` after installation.
- The repaired wheel passed its import/parser/elaboration audit and real
  external-pyslang coexistence tests in both import orders in a clean virtual
  environment.
- Two independent installs generated byte-identical bundled pyslang stubs;
  the packaging gate normalized C++-derived keyword/default-order signatures
  and parsed all six emitted ``.pyi`` files as Python before publishing them.
- A same-build-tree CMake toggle test reported private shared ``naja_slang``
  with interop enabled and restored the ordinary static ``svlang`` target name
  after interop was disabled.

## Continuation protocol

At the beginning of a future session:

1. Read this document and inspect the persistent goal status.
2. Confirm the current branch and working-tree state in every repository being
   modified.
3. Resume the first milestone whose status is `planned` or `in progress`.
4. Update milestone status, decisions, test evidence, and the progress log
   before ending the session.
5. Do not mark the global goal complete until Milestone 7 exit criteria are met.
