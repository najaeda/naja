# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""libpython for executables that embed Python, re-owned by this target."""

load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _python_libs_impl(ctx):
    # cc_binary's dynamic_deps filtering only keeps linker inputs whose
    # owner its graph aspect visits. @rules_python//python/cc:current_py_cc_libs
    # neither advertises CcInfo (so the aspect skips it) nor owns its linker
    # inputs (they belong to a target in the Python toolchain repository,
    # reached only through toolchain resolution). A binary that links
    # naja_runtime dynamically would therefore silently drop libpython. This
    # rule advertises CcInfo and owns the linker inputs, which keeps them.
    cc_info = ctx.attr.libs[CcInfo]
    linker_inputs = [
        cc_common.create_linker_input(
            owner = ctx.label,
            libraries = depset(linker_input.libraries),
            user_link_flags = linker_input.user_link_flags,
            additional_inputs = depset(linker_input.additional_inputs),
        )
        for linker_input in cc_info.linking_context.linker_inputs.to_list()
    ]
    return [CcInfo(
        compilation_context = cc_info.compilation_context,
        linking_context = cc_common.create_linking_context(
            linker_inputs = depset(linker_inputs),
        ),
    )]

python_libs = rule(
    implementation = _python_libs_impl,
    provides = [CcInfo],
    attrs = {
        "libs": attr.label(
            default = "@rules_python//python/cc:current_py_cc_libs",
            providers = [CcInfo],
        ),
    },
)
