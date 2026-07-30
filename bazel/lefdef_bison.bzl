# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Custom Bison codegen rule for the vendored lefdef (LEF/DEF) parsers.

Mirrors thirdparty/lefdef/src/{lef/lef,def/def}/CMakeLists.txt's
BISON_TARGET() calls. Unlike naja-verilog's parser (see
internal/bazel-migration-plan.md), lef.y/def.y use bison's classic C
skeleton (no `%skeleton "lalr1.cc"`), so there's no location.hh-style
auxiliary output to worry about -- just an optional `--defines=` header.

`def` is a deliberate asymmetry, not a bug: def/def/CMakeLists.txt's
BISON_TARGET call omits DEFINES_FILE, so CMake never names an explicit
header, bison then auto-derives an unused "def.hpp" (confirmed
empirically), and every def/def/*.cpp that needs parser token
definitions instead #includes the *already checked into git*
"def.tab.h" -- a historical artifact of how this vendored SDK was
originally committed (see the flagged finding: def/def/ has 67 compiled
.o files and other build byproducts committed alongside source in the
initial "lefdef" vendoring commit, 2025-02-03). This rule replicates
that exact real behavior rather than "fixing" it: `generate_hdr` is only
set for `lef` (which has a real DEFINES_FILE), and `def.tab.h` is listed
as an ordinary checked-in header instead.
"""

def _lefdef_bison_impl(ctx):
    out_cpp = ctx.actions.declare_file(ctx.attr.name + ".cpp")
    outputs = [out_cpp]

    args = ctx.actions.args()
    args.add_all(["-Wconflicts-sr", "-p", ctx.attr.name_prefix])
    args.add("--output=" + out_cpp.path)

    out_hdr = None
    if ctx.attr.hdr_out:
        out_hdr = ctx.actions.declare_file(ctx.attr.hdr_out)
        outputs.append(out_hdr)
        args.add_all(["-d", "--defines=" + out_hdr.path])

    args.add(ctx.file.src)

    ctx.actions.run(
        executable = ctx.executable._bison,
        arguments = [args],
        inputs = [ctx.file.src],
        tools = [ctx.executable._m4],
        outputs = outputs,
        env = {"M4": ctx.executable._m4.path},
        mnemonic = "BisonLefDef",
        progress_message = "Generating lefdef parser from %s" % ctx.file.src.short_path,
    )
    return [DefaultInfo(files = depset(outputs))]

lefdef_bison_parser = rule(
    implementation = _lefdef_bison_impl,
    attrs = {
        "src": attr.label(mandatory = True, allow_single_file = [".y"]),
        "name_prefix": attr.string(
            mandatory = True,
            doc = "bison -p symbol prefix (e.g. lefyy/defyy), avoids " +
                  "yylex/yyparse collisions when linking both parsers.",
        ),
        "hdr_out": attr.string(
            doc = "If set, also generate this --defines= header file. " +
                  "Leave unset to skip header generation entirely (as " +
                  "for `def` -- see file doc comment).",
        ),
        "_bison": attr.label(
            default = "@bison_tool//:bison",
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_m4": attr.label(
            default = "@m4_tool//:m4",
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
    doc = "Generates a lefdef parser .cpp (and optionally a --defines= header) via system bison.",
)
