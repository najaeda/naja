# ADR 0001: Python native runtime interoperability

- Status: accepted
- Date: 2026-09-10
- Scope: `najaeda.naja`, bundled `najaeda.pyslang`, and future
  `pykepler_formal`
- Roadmap: [Python Native Interoperability Roadmap](../python-native-interop-plan.md)

## Context

Naja's SystemVerilog frontend and upstream pyslang both use C++ slang objects.
Najaeda and Kepler Formal likewise operate on C++ Naja objects. Matching source
versions does not make an object created by one independently loaded native
library safe to dereference in another: allocator state, RTTI, singleton state,
type registrations, compiler flags, and object ownership can all differ.

The supported design must therefore make duplicate runtimes detectable before
crossing a pointer. It must also keep the native owner alive for every borrowed
Python view.

## Decision

### Runtime topology

A supported interoperable process has exactly one Naja runtime and one
Naja-owned slang runtime:

- `najaeda.naja` provides the Naja runtime and canonical Python wrappers for
  Naja objects.
- `najaeda.pyslang` and the Naja SystemVerilog frontend link to the same private
  shared slang library built from Naja's pinned slang checkout.
- `pykepler_formal` links to, but does not bundle, the Naja runtime provided by
  najaeda.
- SNL is the hub between source and formal analysis. Kepler does not exchange
  slang pointers directly.

Standalone Naja and Kepler executables may remain self-contained. The single
runtime rule applies to the interoperable Python build mode.

### Native contracts

Native crossings use versioned C-layout function tables stored in private
CPython capsules. The version 1 names are fixed as:

| Provider | Python attribute | Capsule name | Header |
| --- | --- | --- | --- |
| Naja | `najaeda.naja._C_API` | `najaeda.naja._C_API` | `NajaPythonAPI.h` |
| bundled pyslang | `najaeda.pyslang._C_API` | `najaeda.pyslang._C_API` | `NajaPyslangAPI.h` |

The leading underscore is deliberate: these capsules are native extension
contracts, not objects for Python application code.

Every table starts with an API version, table size, build ID, and process-local
runtime identity token. A consumer validates, in order:

1. capsule name through CPython's capsule API;
2. non-null table pointer;
3. exact API version;
4. minimum table size before reading later fields;
5. exact build ID;
6. non-null and pointer-equal runtime identity;
7. every required function pointer.

Only then may it call a table function or cast an opaque pointer to a C++ type.
Any failure becomes `ImportError` during extension initialization or a dedicated
compatibility exception at an explicit connection point. It must never become a
best-effort conversion.

The build ID is an exact compatibility gate in version 1 because neither Naja
nor slang currently promises a stable C++ ABI. It identifies the native build,
not merely the Python distribution version. The runtime identity is the address
of an immutable token with one definition in the shared native runtime. It is
compared by address and is never dereferenced or serialized. This detects two
copies of the same build loaded in one process.

The initial C-layout declarations and validation routines live in:

- `src/nl/python/naja_wrapping/NajaNativeAPI.h`;
- `src/nl/python/naja_wrapping/NajaPythonAPI.h`;
- `src/nl/python/naja_wrapping/NajaPyslangAPI.h`.

Adding fields requires appending them and increasing the API version. Existing
fields, enum values, capsule names, and function signatures are not reordered
or repurposed.

### Ownership and lifetime

Naja owns NLDBs, SNL objects, and retained frontend sessions. A wrapper returned
by the Naja table uses najaeda's existing canonical wrapper type. A consumer
must retain a strong reference to each Python input for the full duration of a
native operation.

Pyslang compilation and symbol wrappers returned for Naja-owned frontend state
are non-owning views. Their bridge functions receive an `owner` Python object;
the returned wrapper must retain that owner. An NLDB cannot release its retained
frontend state while such a view exists. The precise user-facing destruction
behavior will be selected in the frontend-session milestone, but dangling views
are never permitted.

Formal results return canonical najaeda wrappers for SNL objects. They do not
return DNL IDs or pointers because DNL storage is rebuilt and process-global.
Kepler analysis is non-destructive by default and must restore changed global
context through exception-safe guards.

### Pyslang registration isolation

The bundled module is named `najaeda.pyslang`; it does not claim the external
top-level `pyslang` package. Namespace separation alone is insufficient because
pybind11 normally maintains an interpreter-wide C++ type registry.

`module_local` class registration is not sufficient for this binding because
pybind11's native-enum map remains in its interpreter-global internals. The
bundled target therefore compiles all upstream binding translation units in a
private pybind11 C++ namespace and with a private pybind11 internals/conduit
key. This isolates classes, native enums, casters, and inline implementation
symbols without changing the upstream slang checkout.

All Naja-to-pyslang wrapping occurs inside the bundled provider through
`NajaPyslangAPI_v1`; Naja does not ask pybind11 to cast slang objects itself.
CI will import bundled and external pyslang in both orders. The two modules'
Python types remain distinct, and foreign objects must fail bundled-provider
type validation rather than reuse an external registration.

### Diagnostic build information

The following Python functions are part of the planned diagnostic surface. They
do not expose runtime addresses and their dictionaries use these keys:

`najaeda.naja.naja_build_info()`:

| Key | Value |
| --- | --- |
| `provider` | `"najaeda.naja"` |
| `api_version` | integer native table version |
| `naja_version` | Naja release version string |
| `git_hash` | Naja source revision string |
| `build_id` | exact native compatibility build ID |
| `runtime_kind` | `"shared"` for the interoperable build |

`najaeda.pyslang.slang_build_info()`:

| Key | Value |
| --- | --- |
| `provider` | `"najaeda.pyslang"` |
| `api_version` | integer native table version |
| `slang_version` | slang release/version string |
| `git_hash` | pinned slang source revision string |
| `build_id` | exact native compatibility build ID |
| `runtime_kind` | `"shared"` |

These values aid diagnostics and dependency resolvers. They do not replace the
native validation sequence.

## Rejected alternatives

- **Pin only the pyslang Python version.** This says nothing about which native
  image owns a pointer.
- **Compare only Git hashes or semantic versions.** Identical builds can still
  be loaded twice with separate singleton and allocator state.
- **Exchange raw pointer capsules directly.** Capsule names provide a type tag,
  not runtime provenance or lifetime ownership.
- **Let pykepler wrap Naja objects itself.** This creates competing Python type
  identities and makes ownership ambiguous.
- **Statically embed Naja or slang in every extension.** This guarantees the
  duplicate-runtime condition the design needs to prevent.
- **Use only pybind11 `module_local`.** It handles duplicate class bindings but
  does not isolate the native-enum map used by current slang bindings.
- **Use serialized snapshots for interactive calls.** Serialization remains a
  useful process boundary, but loses identity and is not a substitute for the
  requested in-process object exchange.

## Consequences

Wheel construction must preserve shared-library identity during repair on each
platform. Native consumer wheels will have a strict compatibility relationship
with najaeda until a deliberately stable ABI is introduced. The extra checks
make initialization more explicit, but convert likely crashes and corruption
into deterministic Python errors.
