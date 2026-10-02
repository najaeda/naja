# In-tree Bazel registry

naja's `MODULE.bazel` contains only `bazel_dep`s. Modules that are not
(yet) on the [Bazel Central Registry](https://registry.bazel.build/) are
served from this directory, which `.bazelrc` lists ahead of BCR:

```
common --registry=file://%workspace%/bazel/registry
common --registry=https://bcr.bazel.build/
```

| Module | Why it is here |
|---|---|
| `naja-if` | not on BCR |
| `naja-verilog` | not on BCR; the overlay uses BCR `bison`/`flex` instead of host tools |
| `sv-lang` | BCR has slang `f04e815`; naja is developed against `b60d729` |
| `bison` | `3.8.2.bcr.10` is BCR's `bcr.9` with gnulib's wrapper headers on `-I` instead of `-isystem`, so bison builds with toolchains that pass libc headers as `-isystem` (hermetic-llvm); see BCR PR #9600 |

The layout is exactly BCR's (`modules/<name>/metadata.json`,
`modules/<name>/<version>/{MODULE.bazel,source.json,presubmit.yml,overlay/,patches/}`),
so publishing a module is a matter of copying its directory into a
[bazel-central-registry](https://github.com/bazelbuild/bazel-central-registry)
pull request, then deleting it here. As in BCR, `overlay/MODULE.bazel` is
a symlink to the version's `MODULE.bazel`.

## Versions

Unreleased commits use BCR's `<release>-<YYYYMMDD>-<commit>` scheme, e.g.
`0.0.0-20260909-be6544b1`. naja's own build reads the commit back from
that version string (`src/core/naja_version.bzl`).

## Updating a module

1. Add `modules/<name>/<version>/` with a `MODULE.bazel` whose
   `module(version = ...)` matches the directory name, plus any
   `overlay/` files or `patches/` (applied with `-p1`).
2. Add the version to `modules/<name>/metadata.json`.
3. Write `source.json`:

   ```
   bazel/registry/update_source.py <name> <version> <archive-url> [<strip_prefix>]
   ```

   Rerun it after editing an overlay file or patch; it records their
   integrity hashes too.
4. Point the `bazel_dep` in `MODULE.bazel` at the new version.

## Depending on naja from another module

A module that depends on naja needs these registry entries too. Either
copy the module directories into its own registry, or list this one by a
pinned URL:

```
common --registry=https://raw.githubusercontent.com/najaeda/naja/<commit>/bazel/registry/
common --registry=https://bcr.bazel.build/
```
