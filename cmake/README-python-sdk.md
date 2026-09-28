# NajaEDA shared-runtime consumer SDK

The development provider version `0.7.24.dev0` includes this SDK. It is not
compatible with older NajaEDA wheels that do not expose the runtime capsule.
The provider must be released separately before consumers depending on this
version can be published. A local source checkout is not a published release.

## One provider, borrowed objects

`najaeda.naja` remains the owner of the Python types and native runtime. The
extension module loads the `naja_nl`, `naja_dnl`, `naja_bne`, `naja_opt`,
`naja_metrics`, and `naja_python` shared libraries shipped by its wheel.
`NLUniverse` and its stable runtime-identity token reside in `naja_nl`.
Consumers must link to these exact libraries, not compile another Naja copy
or bundle a renamed second copy into their own wheel.

`NajaPythonRuntimeAPI.h` exposes the versioned `najaeda.naja._C_API` capsule.
`NajaPythonRuntime_Import()` validates the C API version and structure size.
Before accessing a borrowed pointer, a C++ consumer must additionally verify:

- The capsule's `build_id` matches `NAJA_RUNTIME_BUILD_ID` from the installed
  SDK's `NajaRuntimeBuild.h`.
- The capsule's `runtime_identity` equals the consumer's linked
  `naja::NL::NLUniverse::getRuntimeIdentity()` token.
- `unwrap_design(object)` succeeds. It accepts this provider's `SNLDesign`
  wrappers and raises `TypeError` for other objects or `ReferenceError` for
  destroyed designs. It does not copy, clone, transfer, or own the design.

Keep the Python owner alive and prevent destruction or concurrent mutation
for the whole native operation. All capsule calls require the GIL, including
the compatibility GIL on free-threaded Python. This API does not make Naja's
global universe thread-safe or support multiple independent provider runtimes.

`naja::DNL::exchange()` detaches/replaces the DNL singleton without creating or
deleting a graph. A scoped borrower can preserve an existing graph while
creating temporary verification graphs. Such a caller must also preserve
the universe/database top selections and the source netlist's order IDs;
exchanging the singleton alone does not restore those values.

## CMake consumption

Install the matching provider wheel into the build interpreter, then obtain
the package location using `najaeda.sdk.get_cmake_dir()`:

```cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
execute_process(
  COMMAND "${Python3_EXECUTABLE}" -c
    "from najaeda.sdk import get_cmake_dir; print(get_cmake_dir())"
  OUTPUT_VARIABLE NajaEDA_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
find_package(NajaEDA CONFIG REQUIRED)
target_link_libraries(my_consumer PRIVATE NajaEDA::runtime)
najaeda_fixup_consumer(my_consumer)
```

The SDK supplies Naja, Verilog constructor, spdlog, and argparse headers.
Matching Boost headers and a TBB development package are required separately.
Configuration rejects different compiler IDs/versions, Python SOABIs, pointer
sizes, Boost versions, and TBB versions. Use the provider's release compiler,
C++ runtime, standard-library ABI, and ABI-affecting build options; a matching
version alone is not a guarantee that arbitrary compiler flags are compatible.
The public C++ interfaces require C++20.

Call `najaeda_fixup_consumer()` for each executable, module, or shared-library
target that links the provider. On macOS it replaces repair-generated absolute
install names (for example `/DLC/...`) in the built consumer with the exact
installed provider paths and re-signs the consumer. It never modifies provider
files. On other platforms the function is a no-op. These build-tree references
are subsequently made relocatable by the consumer's wheel repair step below.

Imported targets retain the familiar names `naja_nl`, `naja_dnl`, `naja_bne`,
`naja_opt`, `naja_metrics`, and `naja_python`. `naja_core` refers to its owning
`naja_nl` runtime; the dump, Liberty, SystemVerilog, Verilog, and visualization
targets refer to their implementations in `naja_python`, not static archives.
The SDK intentionally does not provide the embedded-interpreter CLI loader.

## Repaired wheels

`najaeda.sdk` resolves both original and repair-renamed libraries within the
provider package or its adjacent `najaeda.libs` directory. Ambiguous or missing
Naja runtime libraries are rejected. Bundled `tbb` and `tbbmalloc` libraries
replace the development package's imported library locations while retaining
its version-matched headers, avoiding a second allocator/runtime.

On Windows x86-64, original import libraries are shipped in `sdk/lib`. If wheel
repair renamed a DLL, the SDK reconstructs an import library from that DLL's
named exports, preserving data exports. This requires `llvm-dlltool` or MSVC
`lib.exe`; generated files go in the consumer build directory, never in the
installed package. Other Windows architectures are not currently supported by
that repair path.

Consumer wheel repair must preserve dependencies on the provider's existing
library names and arrange sibling-package loader lookup; it must not copy the
provider libraries into the consumer wheel. Import the provider before loading
the consumer extension. Runtime build/token validation is still required even
after a successful CMake configuration.
